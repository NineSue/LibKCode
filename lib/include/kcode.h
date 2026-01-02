#ifndef KCODE_KCODE_H
#define KCODE_KCODE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus //C++编译器你好，这是C代码,你别碰
extern "C" {

#endif

    typedef struct kcode_rbtree kcode_rbtree_t;
    typedef uint64_t kcode_ref_t;

    int kcode_init(void);
    void kcode_cleanup(void);

    //创建销毁
    kcode_rbtree_t *kcode_rbtree_new(void);
    void kcode_rbtree_free(kcode_rbtree_t *tree);

    //CURD
    int kcode_rbtree_insert(kcode_rbtree_t *tree, uint64_t key,kcode_ref_t handle);
    kcode_ref_t kcode_rbtree_remove(kcode_rbtree_t *tree, uint64_t key);
    kcode_ref_t kcode_rbtree_replace(kcode_rbtree_t *tree, uint64_t key,kcode_ref_t new_ref);
    kcode_ref_t kcode_rbtree_find(kcode_rbtree_t *tree, uint64_t key);

    // 检查 key 是否存在（解决 find 在 ref=0 时的歧义）
    bool kcode_rbtree_contains(kcode_rbtree_t *tree, uint64_t key);

    // 清空所有节点但保留树结构（比 free+new 更高效）
    void kcode_rbtree_clear(kcode_rbtree_t *tree);

    //有序迭代
    kcode_ref_t kcode_rbtree_first(kcode_rbtree_t *tree);
    kcode_ref_t kcode_rbtree_last(kcode_rbtree_t *tree);
    kcode_ref_t kcode_rbtree_prev(kcode_rbtree_t *tree,uint64_t key);
    kcode_ref_t kcode_rbtree_next(kcode_rbtree_t *tree,uint64_t key);

    //后序迭代
    kcode_ref_t kcode_rbtree_first_postorder(kcode_rbtree_t *tree);//最左最深的节点
    kcode_ref_t kcode_rbtree_next_postorder(kcode_rbtree_t *tree,uint64_t key);//找后序的下一个

    size_t kcode_rbtree_size(kcode_rbtree_t *tree);

    //=== sort API ===
    // 与内核 sort() 接口一致的类型定义
    typedef int (*kcode_cmp_func_t)(const void *, const void *);
    typedef int (*kcode_cmp_r_func_t)(const void *, const void *, const void *);
    typedef void (*kcode_swap_func_t)(void *, void *, int);
    typedef void (*kcode_swap_r_func_t)(void *, void *, int, const void *);

    /**
     * kcode_sort - 使用内核 heapsort 对数组排序
     * @base: 数组起始地址
     * @num: 元素个数
     * @size: 每个元素的大小
     * @cmp_func: 比较函数，返回值 <0 表示 a<b, >0 表示 a>b, =0 表示相等
     * @swap_func: 自定义交换函数，NULL 则使用内置优化版本
     */
    void kcode_sort(void *base, size_t num, size_t size,
                    kcode_cmp_func_t cmp_func,
                    kcode_swap_func_t swap_func);

    /**
     * kcode_sort_r - 带私有数据的排序（reentrant 版本）
     * @priv: 传递给 cmp_func 和 swap_func 的私有数据
     */
    void kcode_sort_r(void *base, size_t num, size_t size,
                      kcode_cmp_r_func_t cmp_func,
                      kcode_swap_r_func_t swap_func,
                      const void *priv);

#define KCODE_OK    0
#define KCODE_ENOMEM (-12)
#define KCODE_EXIST (-17)
#define KCODE_EINVAL (-22)
#define KCODE_ENOENT (-2)

#ifdef __cplusplus
}
#endif

#endif //KCODE_KCODE_H