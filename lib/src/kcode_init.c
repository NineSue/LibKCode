/*
 * Copyright (C) 2026 E Zuan, Liu Jiayou, Xia Yefei (Team Xinjian Wenjianjia)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * 项目名称: 内核代码映射用户态
 * 参赛队伍: 新建文件夹 (Xinjian Wenjianjia)
 * 队员: 鄂祖安 (E Zuan), 刘家佑 (Liu Jiayou), 夏业飞 (Xia Yefei)
 */
#include "kcode_ioctl.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include "kcode.h"
#include "../internal/kcode_internal.h"

struct kcode_runtime g_runtime = {.fd = -1, .inited = 0};

/* 1. 工具函数：获取符号信息 */
static int kcode_get_sym_info(int cap_id, struct kcode_sym_info *info) {
    info->cap_id = cap_id;
    return ioctl(g_runtime.fd, KCODE_IOC_GET_SYM, info);
}

/* 2. RB-Tree 映射：复制到用户态内存执行 */
static int kcode_map_rbtree(const int *caps, int count, void **ptrs[]) {
    struct kcode_sym_info infos[count];
    unsigned long min_pfn = ~0UL, max_pfn = 0;

    for (int i = 0; i < count; i++) {
        if (kcode_get_sym_info(caps[i], &infos[i]) < 0) return -1;
        if (infos[i].pfn < min_pfn) min_pfn = infos[i].pfn;
        if (infos[i].pfn > max_pfn) max_pfn = infos[i].pfn;
    }

    size_t total_size = (max_pfn - min_pfn + 1) * 4096;

    // 1. 申请用户态 RWX 内存
    void *u_map = mmap(NULL, total_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (u_map == MAP_FAILED) return -1;

    // 2. 映射内核页用于读取
    void *k_map = mmap(NULL, total_size, PROT_READ, MAP_PRIVATE,
                       g_runtime.fd, min_pfn * 4096);
    if (k_map == MAP_FAILED) {
        munmap(u_map, total_size);
        return -1;
    }

    // 3. 复制内核代码到用户态
    memcpy(u_map, k_map, total_size);
    munmap(k_map, total_size);

    g_runtime.rbtree_base = u_map;
    g_runtime.rbtree_size = total_size;

    for (int i = 0; i < count; i++) {
        *ptrs[i] = (char *)u_map + (infos[i].pfn - min_pfn) * 4096 + infos[i].offset;
    }
    return 0;
}

/* 3. 基础补丁：仅处理 GS 访问，这种情况对于ubuntu莫得问题，只需要击毙金丝雀即可，绝不触碰 Call 指令 */
static void kcode_basic_patch(unsigned char *code, size_t len) {
    for (size_t i = 0; i + 9 <= len; i++) {
        // 匹配模式: 65 48 8b/2b 04 25 28 00 00 00 (mov/sub rax, gs:0x28)
        if (code[i] == 0x65 && code[i+1] == 0x48 &&
            (code[i+2] == 0x8b || code[i+2] == 0x2b) &&
            code[i+3] == 0x04 && code[i+4] == 0x25 &&
            code[i+5] == 0x28 && code[i+6] == 0x00 &&
            code[i+7] == 0x00 && code[i+8] == 0x00) {

            memset(code + i, 0x90, 9);
            i += 8;
        }
    }
}


/* 4. Sort 初始化：干净的搬运逻辑 */
static int kcode_init_sort(void) {
    struct kcode_sym_info s1, s2;
    if (kcode_get_sym_info(KCAP_SORT, &s1) < 0 || kcode_get_sym_info(KCAP_SORT_R, &s2) < 0)
        return -1;

    unsigned long min_pfn = (s1.pfn < s2.pfn) ? s1.pfn : s2.pfn;
    unsigned long max_pfn = (s1.pfn > s2.pfn) ? s1.pfn : s2.pfn;
    size_t sz = (max_pfn - min_pfn + 1) * 4096;

    // 直接在用户空间申请可写内存
    void *u_map = mmap(NULL, sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (u_map == MAP_FAILED) return -1;

    // 从内核驱动读取数据
    void *k_map = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, g_runtime.fd, min_pfn * 4096);
    if (k_map != MAP_FAILED) {
        memcpy(u_map, k_map, sz);
        munmap(k_map, sz);
    } else {
        munmap(u_map, sz);
        return -1;
    }

    // debug 为了反汇编
    // FILE *f = fopen("sort_dump.bin", "wb");
    // if (f) {
    //     fwrite(u_map, 1, sz, f);
    //     fclose(f);
    //     printf("[kcode] Dumped binary to sort_dump.bin\n");
    // }

    // 执行最基本的 GS 补丁
    kcode_basic_patch((unsigned char *)u_map, sz);

    g_runtime.sort = (sort_fn)((char *)u_map + (s1.pfn - min_pfn) * 4096 + s1.offset);
    g_runtime.sort_r = (sort_r_fn)((char *)u_map + (s2.pfn - min_pfn) * 4096 + s2.offset);
    g_runtime.sort_code_page = u_map;
    g_runtime.sort_code_size = sz;
    return 0;
}

/* 5. 总入口 */
int kcode_init(void) {
    if (g_runtime.inited) return 0;
    g_runtime.fd = open("/dev/kcode", O_RDWR);
    if (g_runtime.fd < 0) return -1;

    static const int rb_caps[] = {
        KCAP_RB_INSERT, KCAP_RB_ERASE, KCAP_RB_NEXT, KCAP_RB_PREV,
        KCAP_RB_FIRST, KCAP_RB_LAST, KCAP_RB_FIRST_POSTORDER,
        KCAP_RB_NEXT_POSTORDER, KCAP_RB_REPLACE
    };
    void **rb_ptrs[] = {
        (void**)&g_runtime.rb_insert_color, (void**)&g_runtime.rb_erase,
        (void**)&g_runtime.rb_next, (void**)&g_runtime.rb_prev,
        (void**)&g_runtime.rb_first, (void**)&g_runtime.rb_last,
        (void**)&g_runtime.rb_first_postorder, (void**)&g_runtime.rb_next_postorder,
        (void**)&g_runtime.rb_replace_node
    };

    if (kcode_map_rbtree(rb_caps, 9, rb_ptrs) < 0) goto err;
    if (kcode_init_sort() < 0) goto err;

    g_runtime.inited = 1;
    return 0;
err:
    kcode_cleanup();
    return -1;
}

void kcode_cleanup(void) {
    if (g_runtime.sort_code_page) munmap(g_runtime.sort_code_page, g_runtime.sort_code_size);
    if (g_runtime.rbtree_base) munmap(g_runtime.rbtree_base, g_runtime.rbtree_size);
    if (g_runtime.fd >= 0) close(g_runtime.fd);
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.fd = -1;
}