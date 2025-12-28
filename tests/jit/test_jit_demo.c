/**
 * JIT Demo 测试程序
 * 演示：映射内核代码 → 拷贝 → Patch CALL → 执行
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include "../../kernel/kcode_ioctl.h"

// 用户态替代函数：替换内核的 printk
void user_print_stub(const char *fmt, ...) {
    printf("[USERSPACE] Hijacked kernel printk!\n");
}

/**
 * 此乃失败版本：直接修正相对偏移量 (Direct Relocation)
 * 原理：尝试计算 target 与当前指令的 32 位相对位移。
 * 局限：x86_64 的 E8 (CALL) 指令操作数仅 4 字节，跳转范围限制在 ±2GB。
 * 内核地址空间与用户空间跨度极大，32 位位移必然溢出。
 */
// static int patch_call(void *code_base, uint32_t offset, void *target) {
//     uint8_t *insn = (uint8_t *)code_base + offset;
//
//     if (*insn != 0xE8) {
//         printf("Error: not a CALL at offset %u (found 0x%02x)\n", offset, *insn);
//         return -1;
//     }
//
//     // CALL rel32: 目标 = PC + 5 + rel32
//     // rel32 = 目标 - (PC + 5) = 目标 - insn - 5
//     int64_t rel = (int64_t)target - (int64_t)(insn + 5);
//
//     if (rel > INT32_MAX || rel < INT32_MIN) {
//         printf("Error: target too far (rel=%ld)\n", rel);
//         return -1;
//     }
//
//     int32_t *operand = (int32_t *)(insn + 1);
//     printf("Patch: offset=%u, old_rel=%d, new_rel=%d\n",
//            offset, *operand, (int32_t)rel);
//     *operand = (int32_t)rel;
//
//     return 0;
// }

/**
 * 成功版本：间接跳转跳板 (Trampoline / Relay Patch)
 * 原理：
 * 1. 局部跳转：让原 CALL 指令跳向同页面内的“中转站”（跳板）。
 * 2. 远距离中转：在跳板内利用 64 位寄存器存放绝对地址，实现全空间瞬移。
 */
static int patch_call(void *code_base, uint32_t offset, void *target) {
    uint8_t *insn = (uint8_t *)code_base + offset;

    // 1. 分配跳板空间：在函数末尾（安全区域）构造跳板
    // 注意：offset 128 需要确保不会覆盖原始函数指令
    uint8_t *trampoline = (uint8_t *)code_base + 128;

    // 2. 构建 64 位“绝对跳转”指令序列 (Total 12 bytes):
    // MOVABS RAX, <target_addr> (48 b8 + 8字节地址)
    // JMP RAX                   (ff e0)
    trampoline[0] = 0x48; trampoline[1] = 0xb8;
    *(uint64_t *)(trampoline + 2) = (uint64_t)target;
    trampoline[10] = 0xff; trampoline[11] = 0xe0;

    // 3. 修正原始 CALL 指令：使其指向跳板
    // 此时距离（trampoline - insn）通常极小（< 1KB），32位偏移完全够用
    int64_t rel = (int64_t)trampoline - (int64_t)(insn + 5);

    int32_t *operand = (int32_t *)(insn + 1);
    *operand = (int32_t)rel;

    printf("Fixed! Call at %u now goes to trampoline at %p -> target %p\n",
            offset, trampoline, target);
    return 0;
}

int main(void) {
    int fd = open("/dev/kcode", O_RDWR);
    if (fd < 0) {
        perror("open /dev/kcode");
        return 1;
    }

    // 1. 获取内核函数信息
    struct kcode_sym_info info = { .cap_id = KCAP_JIT_DEMO };
    if (ioctl(fd, KCODE_IOC_GET_SYM, &info) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }
    printf("Got: pfn=0x%lx, offset=0x%lx, len=%zu, relocs=%d\n",
           info.pfn, info.offset, info.len, info.reloc_count);

    // 2. mmap 内核页 (只读)
    void *kernel_page = mmap(NULL, 4096, PROT_READ,
                             MAP_PRIVATE, fd, info.pfn * 4096);
    if (kernel_page == MAP_FAILED) {
        perror("mmap kernel page");
        close(fd);
        return 1;
    }

    // 3. mmap 匿名可写页
    void *exec_page = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (exec_page == MAP_FAILED) {
        perror("mmap exec page");
        munmap(kernel_page, 4096);
        close(fd);
        return 1;
    }

    // 4. 复制代码（从 kernel_page + offset 复制）
    void *code_src = (char *)kernel_page + info.offset;
    memcpy(exec_page, code_src, info.len);
    printf("Copied %zu bytes from kernel to user exec page\n", info.len);

    // 5. Patch 所有 CALL 指令
    for (int i = 0; i < info.reloc_count; i++) {
        printf("Reloc[%d]: offset=%u, target_id=%lu\n",
               i, info.relocs[i].offset, (unsigned long)info.relocs[i].target_id);
        if (info.relocs[i].target_id == 1) {  // 1 = printk
            patch_call(exec_page, info.relocs[i].offset, user_print_stub);
        }
    }

    // 6. 改权限为可执行
    if (mprotect(exec_page, 4096, PROT_READ | PROT_EXEC) < 0) {
        perror("mprotect");
        munmap(exec_page, 4096);
        munmap(kernel_page, 4096);
        close(fd);
        return 1;
    }

    // 7. 执行！
    printf("--- Executing patched kernel code ---\n");
    void (*func)(void) = (void (*)(void))exec_page;
    func();
    printf("--- Returned successfully ---\n");

    // 清理
    munmap(exec_page, 4096);
    munmap(kernel_page, 4096);
    close(fd);

    return 0;
}
