#ifndef KCODE_KCODE_INTERNAL_H
#define KCODE_KCODE_INTERNAL_H

#include <stdint.h>
#include <pthread.h>

#include "kcode.h"

struct rb_node {
    unsigned long __rb_parent_color;//private，存父地址+颜色
    struct rb_node *rb_right;
    struct rb_node *rb_left;
}__attribute__((aligned(sizeof(long))));//内存按照 8 字节对齐


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
    pthread_mutex_t lock;
};

//func point 就只用对应kernel的
typedef void (*rb_insert_color_fn)(struct rb_node *,struct rb_root *);
typedef void (*rb_erase_fn)(struct rb_node *, struct rb_root *);
typedef struct rb_node* (*rb_first_fn)(const struct rb_root  *);
typedef struct rb_node* (*rb_last_fn)(const struct rb_root *);
typedef struct rb_node* (*rb_next_fn)(const struct rb_node *);
typedef struct rb_node* (*rb_prev_fn)(const struct rb_node *);
typedef struct rb_node* (*rb_first_postorder_fn)(const struct rb_root *);
typedef struct rb_node* (*rb_next_postorder_fn)(const struct rb_node *);
typedef void (*rb_replace_node_fn)(struct rb_node *, struct rb_node *,struct rb_root *);

struct kcode_runtime {
    int fd;
    int inited;

    /*
    extern void rb_insert_color(struct rb_node *, struct rb_root *);
    extern void rb_erase(struct rb_node *, struct rb_root *);



    extern struct rb_node *rb_next(const struct rb_node *);
    extern struct rb_node *rb_prev(const struct rb_node *);
    extern struct rb_node *rb_first(const struct rb_root *);
    extern struct rb_node *rb_last(const struct rb_root *);

    extern struct rb_node *rb_first_postorder(const struct rb_root *);
    extern struct rb_node *rb_next_postorder(const struct rb_node *);

    extern void rb_replace_node(struct rb_node *victim, struct rb_node *new,
                    struct rb_root *root);
    */
    rb_insert_color_fn rb_insert_color;
    rb_erase_fn rb_erase;
    rb_next_fn rb_next;
    rb_prev_fn rb_prev;
    rb_first_fn rb_first;
    rb_last_fn rb_last;
    rb_first_postorder_fn rb_first_postorder;
    rb_next_postorder_fn rb_next_postorder;
    rb_replace_node_fn rb_replace_node;

    void *mapping[64];
    int mapping_count;
};

extern struct kcode_runtime g_runtime;

int kcode_map_symbol(int cap_id,void **func_ptr);
#endif //KCODE_KCODE_INTERNAL_H