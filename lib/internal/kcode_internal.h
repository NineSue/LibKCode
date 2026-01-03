#ifndef KCODE_KCODE_INTERNAL_H
#define KCODE_KCODE_INTERNAL_H

#include <stdint.h>
#include <pthread.h>
#include "kcode.h"

struct rb_node {
    unsigned long __rb_parent_color; // private，存父地址+颜色
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long)))); // 内存按照 8 字节对齐

struct rb_root {
    struct rb_node *rb_node;
};

#define RB_ROOT (struct rb_root){.rb_node = NULL}

struct managed_rb_entry {
    struct rb_node node;
    uint64_t key;
    kcode_ref_t ref;
};

struct kcode_rbtree {
    struct rb_root root;
    size_t count;
    pthread_rwlock_t rwlock;
};

// rbtree 函数指针
typedef void (*rb_insert_color_fn)(struct rb_node *, struct rb_root *);
typedef void (*rb_erase_fn)(struct rb_node *, struct rb_root *);
typedef struct rb_node* (*rb_first_fn)(const struct rb_root *);
typedef struct rb_node* (*rb_last_fn)(const struct rb_root *);
typedef struct rb_node* (*rb_next_fn)(const struct rb_node *);
typedef struct rb_node* (*rb_prev_fn)(const struct rb_node *);
typedef struct rb_node* (*rb_first_postorder_fn)(const struct rb_root *);
typedef struct rb_node* (*rb_next_postorder_fn)(const struct rb_node *);
typedef void (*rb_replace_node_fn)(struct rb_node *, struct rb_node *, struct rb_root *);

// sort 函数指针
typedef void (*sort_fn)(void *base, size_t num, size_t size,
                        int (*cmp_func)(const void *, const void *),
                        void (*swap_func)(void *, void *, int));
typedef void (*sort_r_fn)(void *base, size_t num, size_t size,
                          int (*cmp_func)(const void *, const void *, const void *),
                          void (*swap_func)(void *, void *, int, const void *),
                          const void *priv);

struct kcode_runtime {
    int fd;
    int inited;

    // rbtree 函数指针
    rb_insert_color_fn rb_insert_color;
    rb_erase_fn rb_erase;
    rb_next_fn rb_next;
    rb_prev_fn rb_prev;
    rb_first_fn rb_first;
    rb_last_fn rb_last;
    rb_first_postorder_fn rb_first_postorder;
    rb_next_postorder_fn rb_next_postorder;
    rb_replace_node_fn rb_replace_node;

    // [新增] rbtree 连续映射管理
    // 用于记录 map_group 分配的整块虚拟内存，实现 cleanup 时的单次精准释放
    void *rbtree_base;
    size_t rbtree_size;

    // sort 相关 (使用 trampoline 修复后的代码)
    sort_fn sort;
    sort_r_fn sort_r;
    void *sort_code_page;      // 存放修复后的 sort 代码副本
    size_t sort_code_size;     // 副本大小 (通常为 8192)

    // 页面缓存，暂时保留，但已经弃用了
    struct kcode_page_entry {
        unsigned long pfn;
        void *mapped;
    } *page_cache;
    int cache_count;
    int cache_cap;
};

extern struct kcode_runtime g_runtime;

int kcode_map_symbol(int cap_id, void **func_ptr);

#endif //KCODE_KCODE_INTERNAL_H