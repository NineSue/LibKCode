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
#ifndef RBTREE_CHECKER_H
#define RBTREE_CHECKER_H

#include <stdio.h>
#include <pthread.h>
#include "kcode_internal.h"

// === Linux Kernel rbtree 颜色编码 ===
// __rb_parent_color 的最低位存储颜色: 0=Red, 1=Black
// 指针按 sizeof(long) 对齐，所以低位可复用

#define RB_RED      0
#define RB_BLACK    1

#define __rb_parent(pc)    ((struct rb_node *)((pc) & ~3UL))
#define __rb_color(pc)     ((pc) & 1)
#define rb_parent(r)       __rb_parent((r)->__rb_parent_color)
#define rb_color(r)        __rb_color((r)->__rb_parent_color)
#define rb_is_red(r)       (!rb_color(r))
#define rb_is_black(r)     rb_color(r)

// 递归验证节点，返回黑高度，失败返回 -1
static int __verify_rb_node(struct rb_node *node, struct rb_node *parent, int depth) {
    if (!node) {
        // NIL 节点视为黑色，黑高度 = 1
        return 1;
    }

    // 验证父指针
    if (rb_parent(node) != parent) {
        fprintf(stderr, "[FAIL] Parent pointer mismatch at depth %d\n", depth);
        fprintf(stderr, "       node=%p, expected parent=%p, actual parent=%p\n",
                (void*)node, (void*)parent, (void*)rb_parent(node));
        return -1;
    }

    // 红色节点不能有红色子节点
    if (rb_is_red(node)) {
        if ((node->rb_left && rb_is_red(node->rb_left)) ||
            (node->rb_right && rb_is_red(node->rb_right))) {
            fprintf(stderr, "[FAIL] Red violation: red node has red child at depth %d\n", depth);
            return -1;
        }
    }

    int left_bh = __verify_rb_node(node->rb_left, node, depth + 1);
    if (left_bh < 0) return -1;

    int right_bh = __verify_rb_node(node->rb_right, node, depth + 1);
    if (right_bh < 0) return -1;

    // 左右子树黑高度必须相等
    if (left_bh != right_bh) {
        fprintf(stderr, "[FAIL] Black height mismatch at depth %d: left=%d, right=%d\n",
                depth, left_bh, right_bh);
        return -1;
    }

    return left_bh + (rb_is_black(node) ? 1 : 0);
}

// 计算树的实际节点数
static size_t __count_nodes(struct rb_node *node) {
    if (!node) return 0;
    return 1 + __count_nodes(node->rb_left) + __count_nodes(node->rb_right);
}

// 计算树的最大深度
static int __max_depth(struct rb_node *node) {
    if (!node) return 0;
    int left = __max_depth(node->rb_left);
    int right = __max_depth(node->rb_right);
    return 1 + (left > right ? left : right);
}

/**
 * verify_rbtree_integrity - 完整验证红黑树性质
 * @tree: kcode_rbtree_t 指针
 *
 * 验证内容:
 *   1. 根节点是黑色
 *   2. 无连续红节点
 *   3. 所有路径黑高度一致
 *   4. 父指针正确
 *   5. 节点计数一致
 *
 * 返回: 1=通过, 0=失败
 */
static int verify_rbtree_integrity(kcode_rbtree_t *tree) {
    if (!tree) return 1;

    pthread_rwlock_rdlock(&tree->rwlock);

    struct rb_node *root = tree->root.rb_node;

    if (!root) {
        // 空树合法
        if (tree->count != 0) {
            fprintf(stderr, "[FAIL] Empty tree but count=%zu\n", tree->count);
            pthread_rwlock_unlock(&tree->rwlock);
            return 0;
        }
        pthread_rwlock_unlock(&tree->rwlock);
        return 1;
    }

    // 1. 根节点必须是黑色
    if (rb_is_red(root)) {
        fprintf(stderr, "[FAIL] Root is RED\n");
        pthread_rwlock_unlock(&tree->rwlock);
        return 0;
    }

    // 2. 根节点的父指针必须是 NULL
    if (rb_parent(root) != NULL) {
        fprintf(stderr, "[FAIL] Root parent is not NULL\n");
        pthread_rwlock_unlock(&tree->rwlock);
        return 0;
    }

    // 3. 递归验证所有性质
    int bh = __verify_rb_node(root, NULL, 0);
    if (bh < 0) {
        pthread_rwlock_unlock(&tree->rwlock);
        return 0;
    }

    // 4. 验证节点计数
    size_t actual_count = __count_nodes(root);
    if (actual_count != tree->count) {
        fprintf(stderr, "[FAIL] Count mismatch: tree->count=%zu, actual=%zu\n",
                tree->count, actual_count);
        pthread_rwlock_unlock(&tree->rwlock);
        return 0;
    }

    pthread_rwlock_unlock(&tree->rwlock);
    return 1;
}

/**
 * get_rbtree_stats - 获取红黑树统计信息
 */
static void get_rbtree_stats(kcode_rbtree_t *tree, int *black_height, int *max_depth) {
    pthread_rwlock_rdlock(&tree->rwlock);

    struct rb_node *root = tree->root.rb_node;

    if (!root) {
        *black_height = 0;
        *max_depth = 0;
    } else {
        *black_height = __verify_rb_node(root, NULL, 0);
        *max_depth = __max_depth(root);
    }

    pthread_rwlock_unlock(&tree->rwlock);
}

#endif // RBTREE_CHECKER_H
