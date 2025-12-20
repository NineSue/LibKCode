#include "kcode_ioctl.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "kcode.h"
#include "../internal/kcode_internal.h"


struct kcode_runtime g_runtime={.fd = -1,.inited =0};

int kcode_map_symbol(int cap_id,void **func_ptr) {
    struct kcode_sym_info info={.cap_id = cap_id};
    void *mapped;
    size_t map_len;

    if (ioctl(g_runtime.fd,KCODE_IOC_GET_SYM,&info)<0) {
        perror("[kcode_init] ioctl get symbol failed");
        return -1;
    }

    map_len=info.len;
    mapped=mmap(NULL,map_len,PROT_READ|PROT_EXEC,MAP_SHARED,g_runtime.fd,info.pfn*4096);

    if (mapped==MAP_FAILED) {
        perror("[kcode_init] mmap failed");
        return -1;
    }

    *func_ptr=(void*)((char*)mapped+info.offset);
    if (g_runtime.mapping_count<64) {
        g_runtime.mapping[g_runtime.mapping_count++]=mapped;
    }

    printf("[kcode_init]: capID=%d, func=%p,offset=0x%lx\n", cap_id,*func_ptr,info.offset);
    return 0;
}

int kcode_init(void) {
    if (g_runtime.inited)
        return 0;

    g_runtime.fd=open("/dev/kcode",O_RDWR);
    if (g_runtime.fd<0) {
        perror("[kcode_init]: open /dev/kcode");
        return -1;
    }

    if (kcode_map_symbol(KCAP_RB_INSERT,(void **)&g_runtime.rb_insert_color)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_ERASE,(void **)&g_runtime.rb_erase)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_NEXT,(void **)&g_runtime.rb_next)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_PREV,(void **)&g_runtime.rb_prev)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_FIRST,(void **)&g_runtime.rb_first)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_LAST,(void **)&g_runtime.rb_last)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_FIRST_POSTORDER,(void **)&g_runtime.rb_first_postorder)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_NEXT_POSTORDER,(void **)&g_runtime.rb_next_postorder)<0)
        goto err;
    if (kcode_map_symbol(KCAP_RB_REPLACE,(void **)&g_runtime.rb_replace_node)<0)
        goto err;



    g_runtime.inited=1;
    printf("[kcode_init]: runtime initialized\n");
    return 0;

    err:
        kcode_cleanup();
        return -1;
}

void kcode_cleanup(void) {
    for (int i=0;i<g_runtime.mapping_count;i++) {
        if (g_runtime.mapping[i])
            munmap(g_runtime.mapping[i],4096);
    }

    if (g_runtime.fd>0) {
        close(g_runtime.fd);
        g_runtime.fd=-1;
    }

    g_runtime.inited=0;
    printf("[kcode_clean]: cleanup done\n");
}