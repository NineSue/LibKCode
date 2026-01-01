#include "kcode_ioctl.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "kcode.h"
#include "../internal/kcode_internal.h"

#define INITIAL_CACHE_CAP 16

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

    g_runtime.inited=1;
    return 0;

    err:
        kcode_cleanup();
        return -1;
}

void kcode_cleanup(void) {
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