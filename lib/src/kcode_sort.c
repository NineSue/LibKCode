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
