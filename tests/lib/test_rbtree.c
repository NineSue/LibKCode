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

    //traverse
    printf("first:      0x%lx\n",kcode_rbtree_first(tree));
    printf("last:       0x%lx\n",kcode_rbtree_last(tree));

    //del
    kcode_ref_t ref=kcode_rbtree_remove(tree,30);
    printf("remove 30:   0x%lx\nrbtree size:  %zu\n",ref,kcode_rbtree_size(tree));

    kcode_rbtree_free(tree);
    kcode_cleanup();

    printf("test done\n");
}