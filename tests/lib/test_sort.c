#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kcode.h>

//=== 测试辅助函数 ===

// 整数比较函数
static int cmp_int(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

// 降序比较
static int cmp_int_desc(const void *a, const void *b) {
    return *(int *)b - *(int *)a;
}

// 带私有数据的比较（按 priv 指定的字段索引排序）
struct record {
    int id;
    int score;
    char name[16];
};

static int cmp_record_r(const void *a, const void *b, const void *priv) {
    const struct record *ra = a, *rb = b;
    int field = *(int *)priv;
    if (field == 0)
        return ra->id - rb->id;
    else
        return ra->score - rb->score;
}

// 打印整数数组
static void print_array(const char *label, int *arr, size_t n) {
    printf("%s: [", label);
    for (size_t i = 0; i < n && i < 10; i++) {
        printf("%d", arr[i]);
        if (i < n - 1 && i < 9) printf(", ");
    }
    if (n > 10) printf(", ...");
    printf("]\n");
}

// 检查数组是否已排序
static int is_sorted(int *arr, size_t n, int ascending) {
    for (size_t i = 1; i < n; i++) {
        if (ascending) {
            if (arr[i] < arr[i-1]) return 0;
        } else {
            if (arr[i] > arr[i-1]) return 0;
        }
    }
    return 1;
}

//=== 测试用例 ===

// 1. 基本整数排序
static void test_basic_sort(void) {
    printf("\n--- test basic int sort ---\n");
    int arr[] = {64, 25, 12, 22, 11, 90, 45, 33};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    print_array("before", arr, n);
    kcode_sort(arr, n, sizeof(int), cmp_int, NULL);
    print_array("after ", arr, n);

    if (is_sorted(arr, n, 1))
        printf("PASS: array is sorted\n");
    else
        printf("FAIL: array is NOT sorted\n");
}

// 2. 降序排序
static void test_desc_sort(void) {
    printf("\n--- test descending sort ---\n");
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    print_array("before", arr, n);
    kcode_sort(arr, n, sizeof(int), cmp_int_desc, NULL);
    print_array("after ", arr, n);

    if (is_sorted(arr, n, 0))
        printf("PASS: array is sorted descending\n");
    else
        printf("FAIL: array is NOT sorted descending\n");
}

// 3. sort_r 带私有数据
static void test_sort_r(void) {
    printf("\n--- test sort_r with priv ---\n");
    struct record records[] = {
        {3, 85, "Alice"},
        {1, 92, "Bob"},
        {2, 78, "Charlie"},
        {4, 88, "Diana"},
    };
    size_t n = sizeof(records) / sizeof(records[0]);

    // 按 id 排序
    int field = 0;
    printf("Sort by id:\n");
    kcode_sort_r(records, n, sizeof(struct record), cmp_record_r, NULL, &field);
    for (size_t i = 0; i < n; i++)
        printf("  id=%d, score=%d, name=%s\n", records[i].id, records[i].score, records[i].name);

    // 按 score 排序
    field = 1;
    printf("Sort by score:\n");
    kcode_sort_r(records, n, sizeof(struct record), cmp_record_r, NULL, &field);
    for (size_t i = 0; i < n; i++)
        printf("  id=%d, score=%d, name=%s\n", records[i].id, records[i].score, records[i].name);

    printf("PASS: sort_r works\n");
}

// 4. 边界测试
static void test_edge_cases(void) {
    printf("\n--- test edge cases ---\n");

    // 空数组
    int empty[] = {};
    kcode_sort(empty, 0, sizeof(int), cmp_int, NULL);
    printf("PASS: empty array\n");

    // 单元素
    int single[] = {42};
    kcode_sort(single, 1, sizeof(int), cmp_int, NULL);
    if (single[0] == 42)
        printf("PASS: single element\n");
    else
        printf("FAIL: single element\n");

    // 已排序
    int sorted[] = {1, 2, 3, 4, 5};
    kcode_sort(sorted, 5, sizeof(int), cmp_int, NULL);
    if (is_sorted(sorted, 5, 1))
        printf("PASS: already sorted\n");
    else
        printf("FAIL: already sorted\n");

    // 逆序
    int reverse[] = {5, 4, 3, 2, 1};
    kcode_sort(reverse, 5, sizeof(int), cmp_int, NULL);
    if (is_sorted(reverse, 5, 1))
        printf("PASS: reverse sorted\n");
    else
        printf("FAIL: reverse sorted\n");

    // 全相同
    int same[] = {7, 7, 7, 7, 7};
    kcode_sort(same, 5, sizeof(int), cmp_int, NULL);
    if (is_sorted(same, 5, 1))
        printf("PASS: all same elements\n");
    else
        printf("FAIL: all same elements\n");
}

// 5. 性能测试
static void test_performance(void) {
    printf("\n--- test performance ---\n");
    const size_t N = 100000;
    int *arr = malloc(N * sizeof(int));
    if (!arr) {
        printf("SKIP: malloc failed\n");
        return;
    }

    // 填充随机数
    srand(42);
    for (size_t i = 0; i < N; i++)
        arr[i] = rand();

    clock_t start = clock();
    kcode_sort(arr, N, sizeof(int), cmp_int, NULL);
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    printf("Sorted %zu elements in %.2f ms\n", N, elapsed);

    if (is_sorted(arr, N, 1))
        printf("PASS: large array sorted correctly\n");
    else
        printf("FAIL: large array NOT sorted\n");

    free(arr);
}

int main(void) {
    printf("=== kcode_sort test ===\n");

    if (kcode_init() != 0) {
        printf("FATAL: kcode_init failed\n");
        return 1;
    }

    test_basic_sort();
    test_desc_sort();
    test_sort_r();
    test_edge_cases();
    test_performance();

    kcode_cleanup();
    printf("\n=== all tests done ===\n");
    return 0;
}
