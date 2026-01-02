#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kcode.h>
#include "kcode_internal.h"
#include "rbtree_checker.h"

#define TEST_SIZE    100000
#define DELETE_SIZE  50000

// Fisher-Yates 洗牌
static void shuffle(uint64_t *arr, size_t n) {
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        uint64_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void) {
    printf("=== RB-Tree Stress Test ===\n");
    printf("Insert: %d, Delete: %d\n\n", TEST_SIZE, DELETE_SIZE);

    srand(time(NULL));

    if (kcode_init() != 0) {
        fprintf(stderr, "kcode_init failed\n");
        return 1;
    }

    kcode_rbtree_t *tree = kcode_rbtree_new();
    uint64_t *keys = malloc(TEST_SIZE * sizeof(uint64_t));

    // 准备唯一键
    for (int i = 0; i < TEST_SIZE; i++) {
        keys[i] = (uint64_t)i;
    }
    shuffle(keys, TEST_SIZE);

    // === Phase 1: 批量插入 ===
    printf("[1/5] Inserting %d nodes...\n", TEST_SIZE);
    double t0 = now();

    for (int i = 0; i < TEST_SIZE; i++) {
        int ret = kcode_rbtree_insert(tree, keys[i], keys[i]);
        if (ret != KCODE_OK) {
            fprintf(stderr, "Insert failed: key=%lu ret=%d\n", keys[i], ret);
            return 1;
        }
    }

    double t1 = now();
    printf("    %.3fs (%.0f ops/sec)\n", t1 - t0, TEST_SIZE / (t1 - t0));

    // === Phase 2: 验证插入后的树 ===
    printf("[2/5] Verifying after insert...\n");
    if (!verify_rbtree_integrity(tree)) {
        fprintf(stderr, "FAIL: integrity check failed after insert\n");
        return 1;
    }

    int bh, depth;
    get_rbtree_stats(tree, &bh, &depth);
    printf("    OK (nodes=%zu, black_height=%d, max_depth=%d)\n",
           kcode_rbtree_size(tree), bh, depth);

    // === Phase 3: 随机删除 ===
    printf("[3/5] Deleting %d nodes randomly...\n", DELETE_SIZE);
    shuffle(keys, TEST_SIZE);

    t0 = now();
    for (int i = 0; i < DELETE_SIZE; i++) {
        kcode_rbtree_remove(tree, keys[i]);

        // 每 10000 次验证一次
        if ((i + 1) % 10000 == 0) {
            if (!verify_rbtree_integrity(tree)) {
                fprintf(stderr, "FAIL: integrity check failed at delete #%d\n", i + 1);
                return 1;
            }
        }
    }
    t1 = now();
    printf("    %.3fs (%.0f ops/sec)\n", t1 - t0, DELETE_SIZE / (t1 - t0));

    // === Phase 4: 验证删除后的树 ===
    printf("[4/5] Verifying after delete...\n");
    if (!verify_rbtree_integrity(tree)) {
        fprintf(stderr, "FAIL: integrity check failed after delete\n");
        return 1;
    }

    size_t expected = TEST_SIZE - DELETE_SIZE;
    size_t actual = kcode_rbtree_size(tree);
    if (actual != expected) {
        fprintf(stderr, "FAIL: size mismatch, expected %zu got %zu\n", expected, actual);
        return 1;
    }

    get_rbtree_stats(tree, &bh, &depth);
    printf("    OK (nodes=%zu, black_height=%d, max_depth=%d)\n", actual, bh, depth);

    // === Phase 5: 清理 ===
    printf("[5/5] Cleanup...\n");
    kcode_rbtree_free(tree);
    free(keys);
    kcode_cleanup();

    printf("\n=== PASSED ===\n");
    return 0;
}
