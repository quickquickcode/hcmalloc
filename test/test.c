#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "hcmalloc.h"
#include "error.h"



void* test(void* a)
{
    void* ptr[100000];

    for(unsigned long long i=0;i<100000;i++)
    {
        ptr[i]=hcmalloc(0x40);
    }
    
    for(unsigned long long i=0;i<100000;i++)
    {
        hcfree(ptr[i]);
    }



    return NULL;
}

// 测试粒子翻转
void* test_anti()
{
    unsigned long long data1=rand();
    unsigned long long data2=0;
    int size=sizeof(unsigned long long);
    for(int i=0;i<2000;i++)
    {
    //  申请一块抗单粒子翻转的内存
    unsigned long long* ptr=safe_malloc(size);
    
    // 向申请的内存写入数据
    safe_write(ptr,&data1,size);
    
    // 选取掩码
    unsigned long long mask=0xfffefeffefffffef;
    *ptr&=mask;

    // 从申请的内存读取长度0x10的数据
    safe_read(&data2,ptr,size);

    if(data1!=data2)printf("error\n");
    hcfree(ptr);
    }
    printf("particle flip test done\n");
}

#define THREAD_NUM 20


void multi_thread_test()
{
    printf("------func test------\n");
    int flag = 1;
    pthread_t tid[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if (pthread_create(&tid[i], NULL, &test, NULL) < 0)
        {
            printf("pthread_create err\n");
        }
    }
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if (pthread_join(tid[i], NULL) < 0)
        {
            printf("pthread_join err\n");
        }
    }
    printf("test done!\n");
}




int main()
{
    multi_thread_test();
    test_anti();
    return 0;
}
