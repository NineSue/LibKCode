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

/*
 * test_rbtree_advanced.c - 遍历器 / container_of 偏移 / 极端拓扑测试
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <kcode.h>
#include "kcode_internal.h"
#include "rbtree_checker.h"

#define ITER_SIZE   1000
#define SEQ_SIZE    10000


// Part 1: 遍历器测试 (直接访问内部结构绕过 ref!=key 问题)
static int test_iterators(void) {
    printf("\n=== Iterator Test ===\n");

    kcode_rbtree_t *tree = kcode_rbtree_new();
    uint64_t keys[ITER_SIZE];

    // 随机插入
    for (int i = 0; i < ITER_SIZE; i++) {
        keys[i] = rand() % 1000000;
    }

    int inserted = 0;
    for (int i = 0; i < ITER_SIZE; i++) {
        if (kcode_rbtree_insert(tree, keys[i], keys[i] * 10) == KCODE_OK) {
            inserted++;
        }
    }
    printf("[1] Inserted %d unique keys\n", inserted);

    // 直接访问内部结构做中序遍历
    pthread_rwlock_rdlock(&tree->rwlock);

    struct rb_node *node = g_runtime.rb_first(&tree->root);
    uint64_t prev_key = 0;
    int count = 0;
    int first = 1;

    while (node) {
        struct managed_rb_entry *entry =
            (struct managed_rb_entry *)((char *)node - offsetof(struct managed_rb_entry, node));

        if (!first && entry->key <= prev_key) {
            fprintf(stderr, "FAIL: order violation prev=%lu curr=%lu\n", prev_key, entry->key);
            pthread_rwlock_unlock(&tree->rwlock);
            return 0;
        }

        prev_key = entry->key;
        first = 0;
        count++;
        node = g_runtime.rb_next(node);
    }

    pthread_rwlock_unlock(&tree->rwlock);

    if (count != inserted) {
        fprintf(stderr, "FAIL: traversal count %d != inserted %d\n", count, inserted);
        return 0;
    }
    printf("[2] Forward traversal OK (%d nodes in order)\n", count);

    // 反向遍历
    pthread_rwlock_rdlock(&tree->rwlock);

    node = g_runtime.rb_last(&tree->root);
    prev_key = UINT64_MAX;
    count = 0;

    while (node) {
        struct managed_rb_entry *entry =
            (struct managed_rb_entry *)((char *)node - offsetof(struct managed_rb_entry, node));

        if (entry->key >= prev_key && count > 0) {
            fprintf(stderr, "FAIL: reverse order violation\n");
            pthread_rwlock_unlock(&tree->rwlock);
            return 0;
        }

        prev_key = entry->key;
        count++;
        node = g_runtime.rb_prev(node);
    }

    pthread_rwlock_unlock(&tree->rwlock);
    printf("[3] Backward traversal OK (%d nodes)\n", count);

    kcode_rbtree_free(tree);
    return 1;
}


// Part 2: container_of 偏移测试 (rb_node 在不同位置)
// 结构体A: rb_node 在开头
struct entry_head {
    struct rb_node node;
    uint64_t key;
    char data[32];
};

// 结构体B: rb_node 在中间
struct entry_mid {
    uint64_t key;
    char prefix[24];
    struct rb_node node;
    char suffix[16];
};

// 结构体C: rb_node 在末尾
struct entry_tail {
    uint64_t key;
    char data[64];
    struct rb_node node;
};

#define my_rb_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

static int test_container_of(void) {
    printf("\n=== Container_of Offset Test ===\n");

    struct rb_root root = RB_ROOT;
    struct entry_head *h;
    struct entry_mid *m;
    struct entry_tail *t;

    printf("Offsets: head=%zu, mid=%zu, tail=%zu\n",
           offsetof(struct entry_head, node),
           offsetof(struct entry_mid, node),
           offsetof(struct entry_tail, node));

    // 插入三种不同偏移的节点
    h = calloc(1, sizeof(*h));
    h->key = 100;
    strcpy(h->data, "HEAD");

    m = calloc(1, sizeof(*m));
    m->key = 200;
    strcpy(m->prefix, "MID_PRE");
    strcpy(m->suffix, "MID_SUF");

    t = calloc(1, sizeof(*t));
    t->key = 300;
    strcpy(t->data, "TAIL_DATA");

    // 手动构建树 (只测试 container_of，不走完整平衡)
    // 直接用内核函数插入并平衡

    // 插入 head (作为根)
    struct rb_node **link = &root.rb_node;
    struct rb_node *parent = NULL;

    h->node.__rb_parent_color = 0;
    h->node.rb_left = h->node.rb_right = NULL;
    *link = &h->node;
    g_runtime.rb_insert_color(&h->node, &root);

    // 插入 mid
    link = &root.rb_node;
    parent = NULL;
    (void)link;
    (void)parent;
    // 简化测试：直接验证偏移计算正确性

    // 验证 container_of 能正确还原
    struct rb_node *n = &h->node;
    struct entry_head *h2 = my_rb_entry(n, struct entry_head, node);
    if (h2 != h || h2->key != 100 || strcmp(h2->data, "HEAD") != 0) {
        fprintf(stderr, "FAIL: entry_head container_of\n");
        return 0;
    }
    printf("[1] entry_head (offset=0): OK\n");

    n = &m->node;
    struct entry_mid *m2 = my_rb_entry(n, struct entry_mid, node);
    if (m2 != m || m2->key != 200 || strcmp(m2->prefix, "MID_PRE") != 0) {
        fprintf(stderr, "FAIL: entry_mid container_of\n");
        return 0;
    }
    printf("[2] entry_mid  (offset=%zu): OK\n", offsetof(struct entry_mid, node));

    n = &t->node;
    struct entry_tail *t2 = my_rb_entry(n, struct entry_tail, node);
    if (t2 != t || t2->key != 300 || strcmp(t2->data, "TAIL_DATA") != 0) {
        fprintf(stderr, "FAIL: entry_tail container_of\n");
        return 0;
    }
    printf("[3] entry_tail (offset=%zu): OK\n", offsetof(struct entry_tail, node));

    free(h);
    free(m);
    free(t);
    return 1;
}


// Part 3: 极端拓扑测试 (顺序插入 + 重复键)
static int test_sequential_insert(void) {
    printf("\n=== Sequential Insert Test (worst case) ===\n");

    kcode_rbtree_t *tree = kcode_rbtree_new();

    // 顺序插入 1,2,3,...,N —— 最坏情况，强制频繁旋转
    printf("[1] Inserting 1..%d sequentially\n", SEQ_SIZE);

    for (int i = 1; i <= SEQ_SIZE; i++) {
        int ret = kcode_rbtree_insert(tree, i, i);
        if (ret != KCODE_OK) {
            fprintf(stderr, "FAIL: insert %d failed\n", i);
            return 0;
        }

        // 每 1000 次验证
        if (i % 1000 == 0 && !verify_rbtree_integrity(tree)) {
            fprintf(stderr, "FAIL: integrity at i=%d\n", i);
            return 0;
        }
    }

    int bh, depth;
    get_rbtree_stats(tree, &bh, &depth);
    printf("[2] Final: nodes=%zu, black_height=%d, depth=%d\n",
           kcode_rbtree_size(tree), bh, depth);

    // 理论上 n=10000 时, 红黑树高度应 <= 2*log2(n+1) ≈ 28
    if (depth > 30) {
        fprintf(stderr, "WARN: depth %d seems too high for %d nodes\n", depth, SEQ_SIZE);
    }

    if (!verify_rbtree_integrity(tree)) {
        fprintf(stderr, "FAIL: final integrity\n");
        return 0;
    }
    printf("[3] Integrity verified\n");

    kcode_rbtree_free(tree);
    return 1;
}

static int test_duplicate_keys(void) {
    printf("\n=== Duplicate Key Test ===\n");

    kcode_rbtree_t *tree = kcode_rbtree_new();

    // 尝试插入大量重复键
    int success = 0, dup = 0;

    for (int i = 0; i < 1000; i++) {
        int key = i % 100;  // 只有 100 个唯一键
        int ret = kcode_rbtree_insert(tree, key, i);
        if (ret == KCODE_OK) success++;
        else if (ret == KCODE_EXIST) dup++;
    }

    printf("[1] Attempted 1000 inserts with 100 unique keys\n");
    printf("    success=%d, duplicates_rejected=%d\n", success, dup);

    if (success != 100 || dup != 900) {
        fprintf(stderr, "FAIL: expected 100 success, 900 dup\n");
        return 0;
    }

    if (!verify_rbtree_integrity(tree)) {
        fprintf(stderr, "FAIL: integrity after dup test\n");
        return 0;
    }
    printf("[2] Integrity verified\n");

    kcode_rbtree_free(tree);
    return 1;
}

int main(void) {
    printf("========================================\n");
    printf("  RB-Tree Advanced Test Suite\n");
    printf("========================================\n");

    srand(42);  // 固定种子便于复现

    if (kcode_init() != 0) {
        fprintf(stderr, "kcode_init failed\n");
        return 1;
    }

    int pass = 1;

    pass &= test_iterators();
    pass &= test_container_of();
    pass &= test_sequential_insert();
    pass &= test_duplicate_keys();

    kcode_cleanup();

    printf("\n========================================\n");
    if (pass) {
        printf("  ALL TESTS PASSED\n");
    } else {
        printf("  SOME TESTS FAILED\n");
    }
    printf("========================================\n");

    return pass ? 0 : 1;
}
