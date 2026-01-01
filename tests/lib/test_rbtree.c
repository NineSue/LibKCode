#include <stdio.h>
#include <kcode.h>

int main() {
    if (kcode_init()!=0) {
        printf("init failed\n");
        return 1;
    }

    //data ready
    kcode_rbtree_t *tree=kcode_rbtree_new();
    kcode_rbtree_insert(tree,50,0xAABB);
    kcode_rbtree_insert(tree,60,0xCCDD);
    kcode_rbtree_insert(tree,70,0xEEFF);
    kcode_rbtree_insert(tree,80,(kcode_ref_t)"（｀Δ´）！");

    printf("rbtree size:%zu\n",kcode_rbtree_size(tree));

    //find
    printf("find 50:    0x%lx\n",kcode_rbtree_find(tree,50));
    printf("find 70:    0x%lx\n",kcode_rbtree_find(tree,70));
    printf("find 80:    %s\n", (char *)kcode_rbtree_find(tree, 80));

    // 测试 contains()
    printf("\n--- test contains() ---\n");
    printf("contains 50: %s\n", kcode_rbtree_contains(tree, 50) ? "true" : "false");
    printf("contains 99: %s\n", kcode_rbtree_contains(tree, 99) ? "true" : "false");
    // 测试 ref=0 的情况
    kcode_rbtree_insert(tree, 100, 0);  // ref 为 0
    printf("find 100 (ref=0):     0x%lx\n", kcode_rbtree_find(tree, 100));
    printf("contains 100 (ref=0): %s\n", kcode_rbtree_contains(tree, 100) ? "true" : "false");

    //min & max
    printf("\nfirst:      0x%lx\n",kcode_rbtree_first(tree));
    printf("last:       0x%lx\n",kcode_rbtree_last(tree));

    //del
    kcode_ref_t ref=kcode_rbtree_remove(tree,30);
    printf("remove 30:   0x%lx\nrbtree size:  %zu\n",ref,kcode_rbtree_size(tree));

    // 测试 clear()
    printf("\n--- test clear() ---\n");
    printf("before clear: size=%zu\n", kcode_rbtree_size(tree));
    kcode_rbtree_clear(tree);
    printf("after clear:  size=%zu\n", kcode_rbtree_size(tree));
    printf("contains 50 after clear: %s\n", kcode_rbtree_contains(tree, 50) ? "true" : "false");

    // 清空后还能继续用
    kcode_rbtree_insert(tree, 200, 0x1234);
    printf("insert after clear: size=%zu, find 200=0x%lx\n",
           kcode_rbtree_size(tree), kcode_rbtree_find(tree, 200));

    kcode_rbtree_free(tree);
    kcode_cleanup();

    printf("\ntest done\n");
}