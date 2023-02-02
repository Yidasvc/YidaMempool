#include <stdio.h>
#include <sys/time.h>
#include "yida_mempool.h"

#define SIZE 4096
Yida_mempool_t* mpool;

int main(){
    mpool = create_mempool(SIZE);
    struct timeval s1,e1,s2,e2;
    gettimeofday(&s1,NULL);
    for(int i=0;i<30000;i++){
        mempool_alloc(mpool,4);
    }
    gettimeofday(&e1,NULL);
    gettimeofday(&s2,NULL);
    for(int i=0;i<30000;i++){
        malloc(4);
    }
    gettimeofday(&e2,NULL);
    printf("mempool:%lfms ptmalloc:%lfms\n",
        (e1.tv_sec-s1.tv_sec)*1000.0+(e1.tv_usec-s1.tv_usec)/1000.0,
        (e2.tv_sec-s2.tv_sec)*1000.0+(e2.tv_usec-s2.tv_usec)/1000.0);
}