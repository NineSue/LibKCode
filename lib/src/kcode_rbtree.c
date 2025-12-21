#include <kcode.h>
#include <stdlib.h>
#include "kcode_internal.h"

//对象起始地址
#define rb_entry(ptr, type, member) (type *)((char *)(ptr) - offsetof(type, member))

//直接源码拿过来就成
static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,struct rb_node **rb_link)
{
    node->__rb_parent_color = (unsigned long)parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

kcode_rbtree_t *kcode_rbtree_new(void) {
    kcode_rbtree_t *tree=calloc(1,sizeof(kcode_rbtree_t));
    if (!tree) {
        return NULL;
    }

    tree->root= RB_ROOT;
    tree->count=0;
    pthread_rwlock_init(&tree->rwlock,NULL);

    return tree;
}

void kcode_rbtree_free(kcode_rbtree_t *tree) {
    struct rb_node *node,*next;

    if (!tree)
        return;

    pthread_rwlock_wrlock(&tree->rwlock);

    for (node=g_runtime.rb_first_postorder(&tree->root);node;node=next) {
        next=g_runtime.rb_next_postorder(node);
        free(rb_entry(node,struct managed_rb_entry,node));
    }

    pthread_rwlock_unlock(&tree->rwlock);
    pthread_rwlock_destroy(&tree->rwlock);
    free(tree);
}

int kcode_rbtree_insert(kcode_rbtree_t *tree, uint64_t key,kcode_ref_t handle) {
    struct managed_rb_entry *entry,*new_entry;
    struct rb_node **link,*parent=NULL;

    if (!tree||!g_runtime.inited)
        return KCODE_EINVAL;

    new_entry=malloc(sizeof(*new_entry));
    if (!new_entry)
        return KCODE_ENOMEM;

    new_entry->key=key;
    new_entry->ref=handle;
    pthread_rwlock_wrlock(&tree->rwlock);

    link=&tree->root.rb_node;
    while (*link) {
        parent=*link;
        entry=rb_entry(parent,struct managed_rb_entry,node);

        if (key<entry->key) {
            link=&parent->rb_left;
        }else if (key>entry->key) {
            link=&parent->rb_right;
        }else {
            pthread_rwlock_unlock(&tree->rwlock);
            free(new_entry);
            return KCODE_EXIST;
        }
    }

    //把节点丢到树上
    rb_link_node(&new_entry->node,parent,link);

    //调函数表平衡
    g_runtime.rb_insert_color(&new_entry->node,&tree->root);
    tree->count++;
    pthread_rwlock_unlock(&tree->rwlock);

    return KCODE_OK;
}

kcode_ref_t kcode_rbtree_remove(kcode_rbtree_t *tree, uint64_t key) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;
    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_wrlock(&tree->rwlock);
    node=tree->root.rb_node;
    while (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        if (key<entry->key) {
            node=node->rb_left;
        }else if (key>entry->key) {
            node=node->rb_right;
        }else {
            result=entry->ref;
            g_runtime.rb_erase(node,&tree->root);
            free(entry);
            tree->count--;
            break;
        }
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return result;
}

kcode_ref_t kcode_rbtree_replace(kcode_rbtree_t *tree, uint64_t key,kcode_ref_t new_ref) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t old_ref=0;

    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_wrlock(&tree->rwlock);
    node=tree->root.rb_node;
    while (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        if (key<entry->key) {
            node=node->rb_left;
        }else if (key>entry->key) {
            node=node->rb_right;
        }else {
            old_ref=entry->ref;
            entry->ref=new_ref;
            break;
        }
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return old_ref;
}

kcode_ref_t kcode_rbtree_find(kcode_rbtree_t *tree, uint64_t key) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;
    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=tree->root.rb_node;
    while (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        if (key<entry->key) {
            node=node->rb_left;
        }else if (key>entry->key) {
            node=node->rb_right;
        }else {
            result=entry->ref;
            break;
        }
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return result;
}

kcode_ref_t kcode_rbtree_first(kcode_rbtree_t *tree) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;
    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=g_runtime.rb_first(&tree->root);
    if (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        result=entry->ref;
    }
    pthread_rwlock_unlock(&tree->rwlock);

    return result;
}

kcode_ref_t kcode_rbtree_last(kcode_rbtree_t *tree) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;
    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=g_runtime.rb_last(&tree->root);
    if (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        result=entry->ref;
    }
    pthread_rwlock_unlock(&tree->rwlock);

    return result;
}

kcode_ref_t kcode_rbtree_prev(kcode_rbtree_t *tree,uint64_t key) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;

    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=tree->root.rb_node;
    while (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        if (key<entry->key) {
            node=node->rb_left;
        }else if (key>entry->key) {
            node=node->rb_right;
        }else {
            node=g_runtime.rb_prev(node);
            if (node) {
                entry=rb_entry(node,struct managed_rb_entry,node);
                result=entry->ref;
            }
            break;
        }
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return result;
}

kcode_ref_t kcode_rbtree_next(kcode_rbtree_t *tree,uint64_t key) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;

    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=tree->root.rb_node;
    while (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        if (key<entry->key) {
            node=node->rb_left;
        }else if (key>entry->key) {
            node=node->rb_right;
        }else {
            node=g_runtime.rb_next(node);
            if (node) {
                entry=rb_entry(node,struct managed_rb_entry,node);
                result=entry->ref;
            }
            break;
        }
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return result;
}


kcode_ref_t kcode_rbtree_first_postorder(kcode_rbtree_t *tree) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;

    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=g_runtime.rb_first_postorder(&tree->root);
    if (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        result=entry->ref;
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return result;
}

kcode_ref_t kcode_rbtree_next_postorder(kcode_rbtree_t *tree,uint64_t key) {
    struct rb_node *node;
    struct managed_rb_entry *entry;
    kcode_ref_t result=0;

    if (!tree||!g_runtime.inited)
        return 0;

    pthread_rwlock_rdlock(&tree->rwlock);
    node=tree->root.rb_node;
    while (node) {
        entry=rb_entry(node,struct managed_rb_entry,node);
        if (key<entry->key) {
            node=node->rb_left;
        }else if (key>entry->key) {
            node=node->rb_right;
        }else {
            node=g_runtime.rb_next_postorder(node);
            if (node) {
                entry=rb_entry(node,struct managed_rb_entry,node);
                result=entry->ref;
            }
            break;
        }
    }
    pthread_rwlock_unlock(&tree->rwlock);
    return result;
}

size_t kcode_rbtree_size(kcode_rbtree_t *tree) {
    if (!tree)
        return 0;

    return tree->count;
}
