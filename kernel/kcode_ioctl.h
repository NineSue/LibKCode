#ifndef KCODE_IOCTL_H
#define KCODE_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <stddef.h>
#include <sys/ioctl.h>
#endif

//dev
#define KCODE_DEVICE_NAME "kcode"

//id

//rbtree.h
#define KCAP_RB_INSERT          1
#define KCAP_RB_ERASE           2
#define KCAP_RB_FIRST           3  // leftmost node
#define KCAP_RB_LAST            4  // rightmost node
#define KCAP_RB_NEXT            5
#define KCAP_RB_PREV            6
#define KCAP_RB_FIRST_POSTORDER 7  // postorder start
#define KCAP_RB_NEXT_POSTORDER  8  // postorder next
#define KCAP_RB_REPLACE         9

//sort.c
#define KCAP_SORT               10
#define KCAP_SORT_R             11


struct kcode_sym_info {
    int cap_id;
    unsigned long pfn;
    unsigned long offset;
    size_t len;
};

struct kcode_cap_list {
    int count;
    int caps[32];
};

#define KCODE_IOC_MAGIC	'k'
#define KCODE_IOC_GET_SYM _IOWR(KCODE_IOC_MAGIC, 1, struct kcode_sym_info)
#define KCODE_IOC_LIST_CAPS _IOWR(KCODE_IOC_MAGIC, 2, struct kcode_cap_list)

#endif //KCODE_IOCTL_H