#include "kcode_ioctl.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "kcode.h"
#include "../internal/kcode_internal.h"

#define INITIAL_CACHE_CAP 16
#define SORT_CODE_PAGE_SIZE 4096  // sort 代码页大小

// [优化] DEBUG 宏开关控制日志输出
#ifdef KCODE_DEBUG
    #define kcode_log(fmt, ...) printf("[kcode]: " fmt, ##__VA_ARGS__)
#else
    #define kcode_log(fmt, ...) do {} while(0)
#endif

struct kcode_runtime g_runtime={.fd = -1,.inited =0};

// 查找或创建页面映射（按 PFN 去重，动态扩容）
static void *get_or_map_page(unsigned long pfn) {
    // 检查缓存
    for (int i = 0; i < g_runtime.cache_count; i++) {
        if (g_runtime.page_cache[i].pfn == pfn)
            return g_runtime.page_cache[i].mapped;
    }

    // 新建映射
    void *mapped = mmap(NULL, 4096, PROT_READ | PROT_EXEC,
                        MAP_PRIVATE, g_runtime.fd, pfn*4096);
    if (mapped == MAP_FAILED)
        return NULL;

    // 需要扩容
    if (g_runtime.cache_count >= g_runtime.cache_cap) {
        int new_cap = g_runtime.cache_cap * 2;
        struct kcode_page_entry *new_cache = realloc(
            g_runtime.page_cache,
            new_cap * sizeof(struct kcode_page_entry));
        if (!new_cache) {
            munmap(mapped, 4096);
            return NULL;
        }
        g_runtime.page_cache = new_cache;
        g_runtime.cache_cap = new_cap;
    }

    // 存入缓存
    g_runtime.page_cache[g_runtime.cache_count].pfn = pfn;
    g_runtime.page_cache[g_runtime.cache_count].mapped = mapped;
    g_runtime.cache_count++;

    return mapped;
}

int kcode_map_symbol(int cap_id,void **func_ptr) {
    struct kcode_sym_info info={.cap_id = cap_id};

    if (ioctl(g_runtime.fd,KCODE_IOC_GET_SYM,&info)<0) {
        perror("[kcode] ioctl get symbol failed");
        return -1;
    }

    void *page_base = get_or_map_page(info.pfn);
    if (!page_base) {
        perror("[kcode] mmap failed");
        return -1;
    }

    *func_ptr = (char *)page_base + info.offset;
    kcode_log("cap=%d, func=%p (pfn=0x%lx)\n", cap_id, *func_ptr, info.pfn);
    return 0;
}

// 获取符号信息（不映射，只返回 pfn 和 offset）
static int kcode_get_sym_info(int cap_id, struct kcode_sym_info *info) {
    info->cap_id = cap_id;
    if (ioctl(g_runtime.fd, KCODE_IOC_GET_SYM, info) < 0) {
        perror("[kcode] ioctl get symbol failed");
        return -1;
    }
    return 0;
}

/**
 * patch_sort_stack_protector - 移除 sort 函数的栈保护代码
 *
 * 内核 sort() 编译时启用了 stack protector，会访问 gs:0x28（内核栈 canary）
 * 这在用户态无法工作，需要 NOP 掉这些指令
 *
 * 原始代码结构：
 *   0x08-0x10: mov rax, gs:0x28      ; 读取 canary
 *   0x11-0x14: mov [rbp-8], rax      ; 保存 canary
 *   ...
 *   0x30-0x33: mov rax, [rbp-8]      ; 取回 canary
 *   0x34-0x3c: sub rax, gs:0x28      ; 比较 canary
 *   0x3d-0x3e: jne __stack_chk_fail  ; 不匹配则跳转
 *   ...
 *   0x55-0x59: call __stack_chk_fail ; 页外调用
 *
 * Patch 策略：全部替换为 NOP (0x90)
 */
static void patch_sort_stack_protector(unsigned char *sort_code) {
    // Patch 1: [0x08, 0x15) - 13 bytes
    // mov rax, gs:0x28 (9 bytes) + mov [rbp-8], rax (4 bytes)
    memset(sort_code + 0x08, 0x90, 13);

    // Patch 2: [0x30, 0x3f) - 15 bytes
    // mov rax, [rbp-8] (4 bytes) + sub rax, gs:0x28 (9 bytes) + jne (2 bytes)
    memset(sort_code + 0x30, 0x90, 15);

    // Patch 3: [0x55, 0x5a) - 5 bytes
    // call __stack_chk_fail (5 bytes)
    memset(sort_code + 0x55, 0x90, 5);

    kcode_log("sort stack protector patched\n");
}

/**
 * 初始化 sort 函数（使用 trampoline 修复相对跳转）
 *
 * 问题：内核 sort() 内部调用 sort_r() 用的是 E8 相对 CALL 指令
 * 映射到用户态后，相对偏移指向错误地址导致崩溃
 *
 * 解决方案：
 * 1. 分配一块可写可执行的用户态内存
 * 2. 把 sort 和 sort_r 的代码按正确的相对位置复制进去
 * 3. 这样相对 CALL 指令的偏移仍然有效
 */
static int kcode_init_sort(void) {
    struct kcode_sym_info sort_info, sort_r_info;

    // 获取 sort 和 sort_r 的符号信息
    if (kcode_get_sym_info(KCAP_SORT, &sort_info) < 0)
        return -1;
    if (kcode_get_sym_info(KCAP_SORT_R, &sort_r_info) < 0)
        return -1;

    printf("[kcode] sort:   pfn=0x%lx offset=0x%lx\n", sort_info.pfn, sort_info.offset);
    printf("[kcode] sort_r: pfn=0x%lx offset=0x%lx\n", sort_r_info.pfn, sort_r_info.offset);

    // 它们应该在同一页内
    if (sort_info.pfn != sort_r_info.pfn) {
        fprintf(stderr, "[kcode] sort and sort_r are on different pages!\n");
        return -1;
    }

    // 映射内核页（只读）
    void *kernel_page = get_or_map_page(sort_info.pfn);
    if (!kernel_page)
        return -1;

    printf("[kcode] kernel_page mapped at %p\n", kernel_page);

    // 分配用户态可写可执行页
    void *code_page = mmap(NULL, SORT_CODE_PAGE_SIZE,
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code_page == MAP_FAILED) {
        perror("[kcode] mmap code page failed");
        return -1;
    }

    printf("[kcode] code_page at %p\n", code_page);

    // 复制整页代码（保持相对位置不变）
    memcpy(code_page, kernel_page, SORT_CODE_PAGE_SIZE);

    // === 二进制 patch：移除栈保护代码 ===
    patch_sort_stack_protector((unsigned char *)code_page + sort_info.offset);

    // 设置函数指针（在用户态页中的对应位置）
    g_runtime.sort = (sort_fn)((char *)code_page + sort_info.offset);
    g_runtime.sort_r = (sort_r_fn)((char *)code_page + sort_r_info.offset);
    g_runtime.sort_code_page = code_page;
    g_runtime.sort_code_size = SORT_CODE_PAGE_SIZE;

    printf("[kcode] sort func at %p, sort_r func at %p\n",
           (void*)g_runtime.sort, (void*)g_runtime.sort_r);

    // 打印 sort 函数开头几个字节，验证复制是否正确
    unsigned char *sort_bytes = (unsigned char *)g_runtime.sort;
    printf("[kcode] sort first bytes: %02x %02x %02x %02x %02x %02x %02x %02x\n",
           sort_bytes[0], sort_bytes[1], sort_bytes[2], sort_bytes[3],
           sort_bytes[4], sort_bytes[5], sort_bytes[6], sort_bytes[7]);

    return 0;
}

int kcode_init(void) {
    if (g_runtime.inited)
        return 0;

    g_runtime.fd=open("/dev/kcode",O_RDWR);
    if (g_runtime.fd<0) {
        perror("[kcode_init]: open /dev/kcode");
        return -1;
    }

    // 初始化 page cache
    g_runtime.page_cache = malloc(INITIAL_CACHE_CAP * sizeof(struct kcode_page_entry));
    if (!g_runtime.page_cache) {
        close(g_runtime.fd);
        g_runtime.fd = -1;
        return -1;
    }
    g_runtime.cache_cap = INITIAL_CACHE_CAP;
    g_runtime.cache_count = 0;

    if (kcode_map_symbol(KCAP_RB_INSERT,(void **)&g_runtime.rb_insert_color)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_ERASE,(void **)&g_runtime.rb_erase)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_NEXT,(void **)&g_runtime.rb_next)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_PREV,(void **)&g_runtime.rb_prev)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_FIRST,(void **)&g_runtime.rb_first)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_LAST,(void **)&g_runtime.rb_last)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_FIRST_POSTORDER,(void **)&g_runtime.rb_first_postorder)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_NEXT_POSTORDER,(void **)&g_runtime.rb_next_postorder)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_REPLACE,(void **)&g_runtime.rb_replace_node)<0)
        goto err;

    // sort 使用 trampoline 方式初始化（修复相对跳转）
    if (kcode_init_sort() < 0)
        goto err;

    g_runtime.inited=1;
    return 0;

    err:
        kcode_cleanup();
        return -1;
}

void kcode_cleanup(void) {
    // 释放 sort 代码页
    if (g_runtime.sort_code_page) {
        munmap(g_runtime.sort_code_page, g_runtime.sort_code_size);
        g_runtime.sort_code_page = NULL;
        g_runtime.sort = NULL;
        g_runtime.sort_r = NULL;
    }

    for (int i = 0; i < g_runtime.cache_count; i++) {
        if (g_runtime.page_cache && g_runtime.page_cache[i].mapped)
            munmap(g_runtime.page_cache[i].mapped, 4096);
    }
    free(g_runtime.page_cache);
    g_runtime.page_cache = NULL;
    g_runtime.cache_count = 0;
    g_runtime.cache_cap = 0;

    if (g_runtime.fd >= 0) {
        close(g_runtime.fd);
        g_runtime.fd = -1;
    }

    g_runtime.inited = 0;
}