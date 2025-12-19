#include "kcode_ioctl.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/kprobes.h>

static struct {
    int id;
    const char *name;
    size_t size;
}
whitelist[] = {
    {KCAP_RB_INSERT,"rb_insert_color",PAGE_SIZE},
    {KCAP_RB_ERASE,"rb_erase",PAGE_SIZE},
    {KCAP_RB_FIRST,"rb_first",PAGE_SIZE},
    {KCAP_RB_LAST,"rb_last",PAGE_SIZE},
    {KCAP_RB_NEXT,"rb_next",PAGE_SIZE},
    {KCAP_RB_PREV,"rb_prev",PAGE_SIZE},
    {KCAP_RB_FIRST_POSTORDER,"rb_first_postorder",PAGE_SIZE},
    {KCAP_RB_NEXT_POSTORDER,"rb_next_postorder",PAGE_SIZE},
    {KCAP_RB_REPLACE,"rb_replace_node",PAGE_SIZE},
    {0,NULL,0},
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

//给用户态物理页
static long kcode_ioctl(struct file *file,unsigned int cmd,unsigned long arg) {
    struct kcode_sym_info info;
    unsigned long va;
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

    va=kcode_lookup_name(whitelist[i].name);

    if (!va)
        return -ENOENT;

    //转物理页
    info.pfn=slow_virt_to_phys((void *)va)>>PAGE_SHIFT;
    info.offset=va& ~PAGE_MASK;
    info.len=whitelist[i].size;

    if (copy_to_user((void __user *)arg,&info,sizeof(info)))
        return -EFAULT;

    pr_info("[kcode]:%s -> PFN=0x%lx\n",whitelist[i].name,info.pfn);
    return 0;
}

//把物理页映射到用户态虚拟地址
static int kcode_mmap(struct file *file,struct vm_area_struct *vma) {
    unsigned long pfn=vma->vm_pgoff;
    size_t size=vma->vm_end - vma->vm_start;

    if (vma->vm_flags & VM_WRITE)
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
};


static int __init kcode_init(void) {
    int ret=misc_register(&kcode_device);
    if (ret) {
        return ret;
    }
    pr_info("[kcode]:%s ready\n",KCODE_DEVICE_NAME);
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

