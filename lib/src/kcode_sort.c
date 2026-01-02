#include <kcode.h>
#include "kcode_internal.h"

void kcode_sort(void *base, size_t num, size_t size,
                kcode_cmp_func_t cmp_func,
                kcode_swap_func_t swap_func)
{
    if (!g_runtime.inited || !g_runtime.sort)
        return;

    g_runtime.sort(base, num, size, cmp_func, swap_func);
}

void kcode_sort_r(void *base, size_t num, size_t size,
                  kcode_cmp_r_func_t cmp_func,
                  kcode_swap_r_func_t swap_func,
                  const void *priv)
{
    if (!g_runtime.inited || !g_runtime.sort_r)
        return;

    g_runtime.sort_r(base, num, size, cmp_func, swap_func, priv);
}
