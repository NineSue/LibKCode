#include  "kcode.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/mman.h>
int main() {
    int fd;
    struct kcode_sym_info info;
    void *mapped;

    fd=open("/dev/kcode",O_RDWR);//可写可读
    if(fd<0) {
        perror("open /dev/kcode");
        return 1;
    }
    printf("opened /dev/kcode,fd=%d\n",fd);

    int caps[]={
        KCAP_RB_INSERT,
        KCAP_RB_ERASE,
        KCAP_RB_FIRST,
        KCAP_RB_LAST,
        KCAP_RB_NEXT,
        KCAP_RB_PREV,
        KCAP_RB_FIRST_POSTORDER,
        KCAP_RB_NEXT_POSTORDER,
        KCAP_RB_REPLACE
    };
    const char *names[] ={
        "rb_insert_color",
        "rb_erase",
        "rb_first",
        "rb_last",
        "rb_next",
        "rb_prev",
        "rb_first_postorder",
        "rb_next_postorder",
        "rb_replace_node"
    };
    int count=sizeof(caps)/sizeof(caps[0]);

    printf("\n--- ioctl test ---\n");
    for (int i=0; i<count; i++) {
        info.cap_id=caps[i];

        if (ioctl(fd,KCODE_IOC_GET_SYM,&info)<0) {
            perror(names[i]);
            continue;
        }

        printf("%s: pfn=0x%lx,offset=0x%lx,len=%zu\n",
            names[i],info.pfn,info.offset,info.len);
    }

    printf("\n--- mmap test ---\n");
    info.cap_id=KCAP_RB_FIRST;
    if (ioctl(fd,KCODE_IOC_GET_SYM,&info)<0) {
        perror("ioctl rb_first");
        close(fd);
        return 1;
    }

    mapped=mmap(NULL,4096,PROT_READ|PROT_EXEC,MAP_SHARED,fd,info.pfn*4096);
    if (mapped==MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    void *func_addr=(char *)mapped+info.offset;
    printf("mmap success:\n");
    printf("    base    =%p\n",mapped);
    printf("    offset    =0x%lx\n",info.offset);
    printf("    func    =%p\n",func_addr);

    munmap(mapped,4096);
    close(fd);
    printf("\nkcode test done\n");
    return 0;
}