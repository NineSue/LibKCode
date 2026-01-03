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
#include "kcode_ioctl.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/kprobes.h>

static struct {
    int id;
    const char *name;
    unsigned long pfn;     // 预缓存的 PFN
    unsigned long offset;  // 页内偏移
}
whitelist[] = {
    //rb
    {KCAP_RB_INSERT,"rb_insert_color",0,0},
    {KCAP_RB_ERASE,"rb_erase",0,0},
    {KCAP_RB_FIRST,"rb_first",0,0},
    {KCAP_RB_LAST,"rb_last",0,0},
    {KCAP_RB_NEXT,"rb_next",0,0},
    {KCAP_RB_PREV,"rb_prev",0,0},
    {KCAP_RB_FIRST_POSTORDER,"rb_first_postorder",0,0},
    {KCAP_RB_NEXT_POSTORDER,"rb_next_postorder",0,0},
    {KCAP_RB_REPLACE,"rb_replace_node",0,0},
    //sort.c
    {KCAP_SORT,"sort",0,0},
    {KCAP_SORT_R,"sort_r",0,0},
    {0,NULL,0,0},
};

//查完符号对应的va就扯呼，嘿嘿
//因为从linux 5.7后，原本做这件事得kallsyms_lookup_name不再给内核模块用了
static unsigned long kcode_lookup_name(const char * name){
    struct kprobe kp={.symbol_name = name};
    unsigned long addr;

    if (register_kprobe(&kp)<0)
        return 0;

    addr=(unsigned long)kp.addr;
    unregister_kprobe(&kp);
    return addr;
};

//给用户态物理页（直接返回缓存值）
static long kcode_ioctl(struct file *file,unsigned int cmd,unsigned long arg) {
    struct kcode_sym_info info;
    int i;

    if (cmd!=KCODE_IOC_GET_SYM)
        return -ENOTTY;

    if (copy_from_user(&info,(void __user*)arg,sizeof(info)))
        return -EFAULT;

    for (i = 0; whitelist[i].name; ++i) {
        if (whitelist[i].id==info.cap_id)
            break;
    }

    if (!whitelist[i].name)
        return -EINVAL;

    // 直接使用预缓存的值
    if (!whitelist[i].pfn)
        return -ENOENT;

    info.pfn=whitelist[i].pfn;
    info.offset=whitelist[i].offset;
    info.len=PAGE_SIZE;

    if (copy_to_user((void __user *)arg,&info,sizeof(info)))
        return -EFAULT;

    return 0;
}

//验证 PFN 是否在白名单中
static bool kcode_pfn_valid(unsigned long pfn) {
    int i;
    for (i = 0; whitelist[i].name; ++i) {
        if (whitelist[i].pfn == pfn)
            return true;
    }
    return false;
}

//把物理页映射到用户态虚拟地址
static int kcode_mmap(struct file *file,struct vm_area_struct *vma) {
    unsigned long pfn=vma->vm_pgoff;
    size_t size=vma->vm_end - vma->vm_start;

    if (vma->vm_flags & VM_WRITE)
        return -EPERM;

    // 验证 PFN 是否合法
    if (!kcode_pfn_valid(pfn))
        return -EPERM;

    return remap_pfn_range(vma,vma->vm_start,pfn,size,vma->vm_page_prot);
}

static const struct file_operations kcode_fops = {//NNOLINT
    .owner=THIS_MODULE,
    .unlocked_ioctl=kcode_ioctl,
    .mmap=kcode_mmap,
};

static struct miscdevice kcode_device = {
    .minor=MISC_DYNAMIC_MINOR,
    .name = KCODE_DEVICE_NAME,
    .fops = &kcode_fops,
    .mode=0666,//no sudo
};


//预缓存所有符号地址
static int kcode_cache_symbols(void) {
    int i;
    unsigned long va;

    for (i = 0; whitelist[i].name; ++i) {
        va = kcode_lookup_name(whitelist[i].name);
        if (!va) {
            pr_warn("[kcode]: failed to lookup %s\n", whitelist[i].name);
            continue;
        }
        whitelist[i].pfn = slow_virt_to_phys((void *)va) >> PAGE_SHIFT;
        whitelist[i].offset = va & ~PAGE_MASK;
        pr_info("[kcode]: %s -> PFN=0x%lx offset=0x%lx\n",
                whitelist[i].name, whitelist[i].pfn, whitelist[i].offset);
    }
    return 0;
}

static int __init kcode_init(void) {
    int ret;

    // 先缓存符号
    kcode_cache_symbols();

    ret=misc_register(&kcode_device);
    if (ret) {
        return ret;
    }
    pr_info("[kcode]: %s ready\n",KCODE_DEVICE_NAME);
    return 0;
}

static void __exit kcode_exit(void) {
    misc_deregister(&kcode_device);
    pr_info("[kcode]:unload device\n");
}

module_init(kcode_init);
module_exit(kcode_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("an");

