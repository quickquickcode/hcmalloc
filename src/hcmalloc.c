#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "hcmalloc.h"
#include "error.h"
#include <stdint.h>


/*
  -----------------------  全局变量 -----------------------
*/




void *__start_addr;
void* __end_addr;
unsigned int __book_num; 
page* __free_book_list;
pthread_once_t global_once = PTHREAD_ONCE_INIT;

lock_t __book_lock;


/*
  -----------------------  预存信息 -----------------------
*/


const unsigned char G[8] = { 0b00101011, 0b10010101, 0b11001010, 0b01100101, 0b10110010, 0b01011001, 0b10101100, 0b01010110 };
const unsigned char H[8] = { 0b01101010, 0b00110101, 0b10011010, 0b01001101, 0b10100110, 0b01010011, 0b10101001, 0b11010100 };

 

const char idx2[2169] = { 0,0,0,0,1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,0,12,0,13,0,14,0,15,0,0,0,16,0,0,0,17,0,0,0,18,0,0,0,19,0,0,0,20,0,0,0,21,0,0,0,22,0,0,0,23,0,0,0,0,0,0,0,24,0,0,0,0,0,0,0,25,0,0,0,0,0,0,0,26,0,0,0,0,0,0,0,27,0,0,0,0,0,0,0,28,0,0,0,0,0,0,0,29,0,0,0,0,0,0,0,30,0,0,0,0,0,0,0,31,32,33,34,35,36,36,37,37,0,38,0,39,0,40,0,41,0,42,0,43,0,43,0,43,0,0,0,44,0,0,0,45,0,0,0,46,0,0,0,46,0,0,0,47,0,0,0,48,0,0,0,48,0,0,0,48,0,0,0,0,0,0,0,49,0,0,0,0,0,0,0,50,0,0,0,0,0,0,0,51,0,0,0,0,0,0,0,51,0,0,0,0,0,0,0,52,0,0,0,0,0,0,0,53,0,0,0,0,0,0,0,53,0,0,0,0,0,0,0,53,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,54,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,54,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,57,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,57,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,57,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,58,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,58,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,59,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,59,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,61,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,61,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,67,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,68,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,69,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,71,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,73,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,74,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,76,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,77,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,78,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,79,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,81,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,82,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,83,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85 };

const size_t idx_to_size[86] = { 16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256,288,320,352,384,416,448,480,512,576,640,704,768,
832,896,960,1024,1152,1280,1408,1536,1792,2048,2304,2560,2816,3072,3328,4096,4608,5120,6144,6656,8192,9216,10240,12288,13312,16384,
20480,24576,26624,32768,40960,49152,57344,65536,73728,81920,90112,98304,106496,114688,122880,131072,139264,147456,155648,163840,
172032,180224,188416,196608,204800,212992,221184,229376,237568,245760,253952,262144 };

const size_t idx_to_pages[86] = { 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,6,4,6,4,6,4,6,10,4,10,
8,6,10,4,10,6,14,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64 };

/*
  --------------------------  TLS -------------------------
*/

pthread_key_t p_key;
__thread pthread_once_t thread_once = PTHREAD_ONCE_INIT;
__thread thread_cache tcache;
__thread struct error_s error;


/*
  --------------------------  函数 -------------------------
*/


void* check_init()
{
    pthread_once(&thread_once,thread_init);
}
#define add_num 1

void global_init()
{

    // 初始化锁
    lock_init(&__book_lock);
    pthread_key_create(&p_key,tcache_destroy);     
    __free_book_list=NULL;

    int num=add_num*4;
    // 上锁
    lock(&__book_lock);

    // 改写全局变量中book个数和起始结束地址,向系统申请空间
    __start_addr = sbrk(BOOK_SIZE*num);
    // 对齐
    sbrk((1<<BOOK_SHIFT) - (((size_t)__start_addr&((1<<BOOK_SHIFT)-1))) );
    __start_addr = (void*)((((size_t)__start_addr>>BOOK_SHIFT)+1)<<BOOK_SHIFT);

    __end_addr = __start_addr+BOOK_SIZE*num;
    __book_num=num;

    page* p=(page*)__start_addr;
    for (int i = 0; i < num-1; i++)
    {
        p->next=__free_book_list;
        __free_book_list=p;
        p=p+512;
    }
    __book_num+=num;
    __end_addr+=BOOK_SIZE*num;

    // 解锁
    unlock(&__book_lock);
} 


void thread_init()
{
    // 检查全局初始化
    pthread_once(&global_once,global_init);
    // 初始化现场局部变量
    pthread_setspecific(p_key,&tcache);

    tcache._book_list=NULL;
    tcache._book_num=0;
    
    /*
        初始线程缓存
    */
    for(int i=0;i<SIZE_CLASS_NUM;i++)
    {
        block_bin(i).next=&block_bin(i);
        block_bin(i).prev=&block_bin(i);
        tcache._above_times[i]=0;
        tcache._span_num[i]=0;
        tcache._need_free_list=NULL;
    }
    for(int i=0;i<9;i++)
    {
        tcache._span_bin[i].next=&tcache._span_bin[i];
        tcache._span_bin[i].prev=&tcache._span_bin[i];                 
    }
    
    // 向中央申请book
    trans_book(add_num);
    page* tmp=tcache._book_list;
    while (tmp->next)
    {
        tmp=tmp->next;
    }
    tcache._last_book=tmp;
    
    return;
}

void tcache_destroy(void* arg)
{
    //if(!trylock(&__book_lock))
    {
    lock(&__book_lock);
    //while (tcache._book_list)
    //{
    //    page* tmp=tcache._book_list;
    //    tcache._book_list=tcache._book_list->next;
    //    // ret_book(tmp);
    //    tmp->next=__free_book_list;
    //    __free_book_list=tmp;
    //}
    tcache._last_book->next=__free_book_list;
    __free_book_list=tcache._book_list;
    unlock(&__book_lock);
    }
}

static inline void add_book(int num)
{
    page* p=sbrk(BOOK_SIZE*num);
    page* tmp=p;
    for (int i = 0; i < num-1; i++)
    {
        //  book_init(p);
        // p->next=p+512;
        // p=p->next;

        p->next=__free_book_list;
        __free_book_list=p;

        p=p+512;
    }

    //p->next=__free_book_list;
    //__free_book_list=tmp;

    __book_num+=num;
    __end_addr+=BOOK_SIZE*num;
    return;
}


static inline void *book_init(page* victim)
{
    // 初始化book的指针
    victim->prev=NULL;
    // 修改header page中的header

    victim->page_header[0].block_size_idx=255;
    victim->page_header[0].hup=0b101;
    victim->page_header[0].span_len=511;
    victim->page_header[0].prev_span_len=0;
}

static inline void trans_book(int num)
{
    int got_num = get_book(num);
    page* tmp=tcache._book_list;
    for(int i=0;i<got_num;i++)
    {
        book_init(tmp);
        //link_span(tmp+1);
        bin* p=(bin*)(tmp+1);
        tmp=tmp->next;
        tcache._span_bin[8].prev->next=p;
        p->next=&tcache._span_bin[8];
        p->prev=tcache._span_bin[8].prev;
        tcache._span_bin[8].prev=p;
    }
    return;
}


static inline int get_book(int num)
{
    // 向中央申请book
    lock(&__book_lock);
get_book_trans: ;
    // 检查是否有book链表,没有就添加book
    if(__free_book_list!=NULL)
    {
        int i;
        for (i=0; i < num; )
        {
            if(__free_book_list==NULL)break;
            i++;
            // 从主链中弹出一个book
            page* tmp=__free_book_list;
            __free_book_list=__free_book_list->next;
            

            // 将这个book初始化好
            //book_init(tmp);
            // book放入线程缓存中
            tmp->next=tcache._book_list;
            tcache._book_list=tmp;
            //tcache._book_num++;

            // tmp转到span的头部
            //page* p=tmp+1;
            //link_span((bin*)p);
            
        }
        unlock(&__book_lock);
        return i;
    }
    
    // 没有book，中央向系统申请book
    add_book(num*4);
    goto get_book_trans;
}

static inline void *ret_book(page* book_addr)
{
    page* tmp=book_addr;
    tmp->next=__free_book_list;
    __free_book_list=tmp;
}

void* hcmalloc(size_t size)
{
    // 初始化
    check_init();
    // 判断大小
    if(size<=0)
    {
        return NULL;
    }
    // 返回指针
    void* rt=NULL;
    // 小内存申请block
    if(size <= BLOCK_MAX_SIZE)
    {
        rt=small_malloc(size);
        goto malloc_judge;
    }
    // 大内存申请span
    if(size <= SPAN_MAX_SIZE)
    {
        size_t page_num = (size>>PAGE_SHIFT)+1;       // 获取申请的span大小
        rt=big_malloc(page_num);
        goto malloc_judge;
    }
    // 超大内存使用系统调用    
    huge_malloc(size);
    return rt;

    // 判断指针是否跳出堆地址，记录攻击
malloc_judge: ;

#ifdef Safe
    if(rt<__start_addr|rt>__end_addr)
    {
        log_error(ATTACK);
        return NULL;
    }
#endif

    return rt;
}

static inline void* small_malloc(size_t size)
{
    // 获取对应的大小下标
    int idx = size2idx(size);
    // 帮助size对齐到块大小
    size=idx2size(idx);     
    // 返回的指针    
    block* rt = NULL;
    // 分配对应的idx大小编号的block
    if(check_block(idx))
    {
        rt=pop_block(idx);
        get_span_header(rt)->use_num++;
        return rt;
    }

    // 分配失败
#ifdef Over_alloc
    // 如果当前链表大小的块为空,而且超额次数没有达到 选择超额分配
    if(tcache._above_times[idx]<2)
    {   // 向下超额分配两次
        for(int i=idx+1; i < SIZE_CLASS_NUM && i <= idx+2; i++)
        {
            if(check_block(i))
            {
                tcache._above_times[idx]++;
                rt=pop_block(i);
                get_span_header(rt)->use_num++;
                return rt;
            }
        }
    }
#endif

    // 没有空闲块
    // 向线程缓存申请span
    int page_num = get_page_num(idx);
    char* victim=big_malloc(page_num);
    tcache._span_num[idx]++;

    // 改写header
    header* tmp_header=page2header(victim);
    tmp_header->block_size_idx=idx;   
    tmp_header->use_num=0;

    // 从span切割的block放入垃圾桶中
    int num=page_num*PAGE_SIZE/size;
    for (int i = 0; i < num; i++)
    {
        push_block2bin((block*)victim,idx);
        victim+=size;
    }
    
    rt=pop_block(idx);
    tmp_header->use_num++;
    return rt;
}

static void* big_malloc(size_t page_num)
{
    bin* tmp;
big_malloc_get_span: ;
    // 在span_bin中寻找可以切割的span
    for(int i=max_bit(page_num);i<9;i++)
    {
        tmp=tcache._span_bin[i].next;
        while(tmp != &tcache._span_bin[i])
        {   
            if(page_num <= page2header(tmp)->span_len)
            {
                unlink_span((page*)tmp);
                goto big_malloc_span_cut;
            }
            tmp=tmp->next;
        }
    }
    trans_book(add_num);
    goto big_malloc_get_span;

big_malloc_span_cut: ;
    header* p1=page2header(tmp);
    unsigned int tmp_size = p1->span_len;
    p1->block_size_idx=255;
    p1->hup=0b111;
    p1->span_len=page_num;
    for (int i = 1; i < page_num; i++)
    {
        p1[i].hup=0;
        p1[i].span_len=i;       // 在非span头部，这个变量为距离
    }
    
    header* p2=p1+page_num;
    p2->hup=0b101;
    p2->span_len=tmp_size-page_num;
    p2->prev_span_len=page_num;
    for (unsigned int i = 1; i < tmp_size-page_num; i++)
    {
        p2[i].hup=0;
        p2[i].span_len=i;
    }
    link_span((bin*)header2page(p2));
    return header2page(p1);

}

static void *huge_malloc(size_t size)
{
    void *addr;
    size_t tot_size = size + sizeof(int);
    //调用mmap获取内存
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    *(int *)addr = size+sizeof(int);
    return addr + sizeof(int);
}


void hcfree(void *addr)
{       
    if(addr==NULL)return;
    if(addr<__start_addr | addr>__end_addr)
    {   
        huge_free(addr);
        return;
    }
    // 链入待释放链表
    ((block*)addr)->next=tcache._need_free_list;
    tcache._need_free_list=addr;
    // 增加个数
    tcache._need_free_num++;
    // 待释放链表个数小于十，直接返回
    if (tcache._need_free_num<1000)return;



    // 处理待释放链表,当其头指针不为空
    while (tcache._need_free_list)
    {
        // 获取头指针
        block* victim=tcache._need_free_list;
        tcache._need_free_list=tcache._need_free_list->next;

        // 获取这个block所在page的header
        header* page_header=page2header(victim);
        // 这个page所在span的第一个header
        header* span_header=get_span_header(victim);
        // 获取该块的大小下标
        int block_idx=span_header->block_size_idx;

        // 小内存释放,直接放入对应链表，
        // 如果该span的所有块都被释放，则进入大内存释放
        if(block_idx<SIZE_CLASS_NUM)
        {
            push_block2bin(victim,block_idx);
            span_header->use_num--;
        }
        // 大内存释放
        else if(block_idx>=SIZE_CLASS_NUM)
        {
        
        #ifdef Safe
            if(page_header!=span_header) 
            {
                log_error(BAD_PTR);
                return;
            }
        #endif
        
            big_free(span_header);
        }
        // 跳转到下一个待释放的块
    }
    tcache._need_free_list=NULL;
    tcache._need_free_num=0;
    return;
}

static inline void small_free(void *addr,int idx)
{
    push_block2bin(addr,idx);
}

static inline void big_free(header* p)
{
    // 如果前一个没有在使用
    // 将这个两个合并
    if(p->hup&1==0)
    {
        header* prev_span=p - p->span_len;
        prev_span->span_len+=p->span_len;
        unlink_span(header2page(prev_span));
        p=prev_span;   
    }

    header* next_span=p + p->span_len;
    // 如果本span是这个book最后一个span，那么下一个的地址应该是失效的地址
    if((uintptr_t)(next_span)&PAGE_MASK)
    {
        // 如果下一个span没有使用,则合并
        if(next_span->hup&0b10==0)
        {
            unlink_span(header2page(next_span));            // !!!!
            p->span_len+=next_span->span_len;
        }
    }
    for (int i = 1; i < p->span_len; i++)
    {
            p[i].hup=0;
            p[i].span_len=i;
    }
    // 再次查看下一个
    next_span=p + p->span_len;
    if((uintptr_t)(next_span)&PAGE_MASK)
    {
        next_span->hup=0b110;
        next_span->prev_span_len=p->span_len;
    }
    link_span((bin*)header2page(p));
    return;
}

static void huge_free(void *ptr)
{
    void *temp = ptr - sizeof(int);
    //调用unmmap释放内存
    int size = *(int *)temp;
    munmap(temp, size);
}

void* hcrealloc(void *addr,size_t size)
{
    if(addr<__start_addr|addr>__end_addr)
    {
        hcfree(addr);
        return hcmalloc(size);
    }
    else
    {
        int addr_idx=get_span_header(addr)->block_size_idx;
        if(addr_idx<SIZE_CLASS_NUM)
        {
           if(addr_idx>=size2idx(size)) return addr;
           else
           {
                hcfree(addr);
                return hcmalloc(size);
           }
        }
        else
        {
            int new_page_num = (size>>PAGE_SHIFT)+1;
            if(get_span_header(addr)->span_len >= new_page_num) return addr;
            else
            {
                hcfree(addr);
                return hcmalloc(new_page_num);
            }
        }
    }
}

void* hccalloc(size_t size)
{
    void* rt = hcmalloc(size);
    memset(rt,'\x00',size);
    return rt;
}

void* safe_malloc(size_t size)
{
    return hcmalloc(size*2);
}

void* safe_write(void* ptr,void* data,size_t size)
{
    memcpy(ptr,data,size);
    int block_size=get_span_header(ptr)->block_size_idx;
    block_size=idx2size(block_size)/2;
    note_data(ptr,ptr+block_size,size);
    return ptr;
}

void* safe_read(void* w_addr,void* ptr,size_t size)
{
    int block_idx=get_span_header(ptr)->block_size_idx;
    int block_size= 0;
    if(block_idx <SIZE_CLASS_NUM)
    {
        block_size=idx2size(block_idx)/2;
    }
    else
    {
        block_size=(get_span_header(ptr)->span_len*PAGE_SIZE)/2;
    }
    check_data(ptr,ptr+block_size,size);
    memcpy(w_addr,ptr,size);
}

static inline void link_span(bin* p)
{
    int p_span_len=page2header(p)->span_len;

    int idx = max_bit(page2header(p)->span_len);
    bin* tmp=tcache._span_bin[idx].next;
    // 按大小顺序放入
    while (tmp != &tcache._span_bin[idx])
    {
        if(p_span_len <= page2header(tmp)->span_len)
        {
            p->next=tmp;
            tmp->prev->next=p;
            p->prev=tmp->prev;
            tmp->prev=p;
            return;
        }
        tmp=tmp->next;
    }
    // 该大小的bin没有比这个更大的span，链接在最后一个
    tcache._span_bin[idx].prev->next=p;
    p->next=&tcache._span_bin[idx];
    p->prev=tcache._span_bin[idx].prev;
    tcache._span_bin[idx].prev=p;
    return;
}

static inline void unlink_span(page* p)
{
  p->prev->next=p->next;
  p->next->prev=p->prev;
}

static inline int check_block(int idx)
{
    return &block_bin(idx)==block_bin(idx).next ? 0 : 1;
}

static inline block* pop_block(int idx)
{
    block* tmp=block_bin(idx).next;
    unlink_block(tmp);
    return tmp;
}

static inline void push_block2bin(block* p,int idx)
{
    p->next=block_bin(idx).next;
    p->prev=&block_bin(idx);
    p->next->prev=p;
    p->prev->next=p;
    return;
}

static inline void unlink_block(block* p)
{
    p->prev->next=p->next;
    p->next->prev=p->prev;
    return;
}

static inline header* get_span_header(void *addr)
{
  header* tmp=page2header(addr);
  if(tmp==NULL)return NULL;
  header* rt=tmp->hup ? tmp : tmp-(tmp->span_len); 
  //header* rt; 
  //if(tmp->hup)rt=tmp;
  //else rt=tmp-(tmp->span_len);
  return rt;
}

static inline int class_size_id(int size)
{
    int alignment = kAlignment;
	if (size >= 128) { 
		// Space wasted due to alignment is at most 1/8, i.e., 12.5%.
		alignment = (1 << max_bit(size)) / 8;
	    
        // Maximum alignment allowed is page size alignment.
	    if (alignment > PAGE_SIZE) {
	    	alignment = PAGE_SIZE;
	    }
	}

	// 计算对齐所需的偏移量
	int s = (size & ((alignment-1))) ? (size & (~(alignment-1))) + alignment : (size & (~(alignment-1)));

	const char big = (s > kMaxSmallSize);
	const int add_amount = big ? (127 + (120 << 7)) : 7;
	const int shift_amount = big ? 7 : 3;

	int id1=(s + add_amount) >> shift_amount;
	return idx2[id1]-0;
}

static void note_data(char* addr1, char* addr2, int size)
{
	for (int j = 0; j < size; j++)
	{
		unsigned char M = *(addr1 + j);
		// 生成校验数据A
		unsigned char A = 0;
		for (int i = 0; i < 8; i++)
		{
			A <<= 1;
			//计算1的个数（微软_mm_popcnt_u32）（gcc __builtin_popcount）(M & G[i])
			A += ((__builtin_popcount(M & G[i])) % 1);
		}
		*(addr2 + j) = A;
	}
}

static void check_data(char* addr1, char* addr2, int size)
{
	for (int k = 0; k < size; k++)
	{
		unsigned char M = *(addr1 + k);
		unsigned char A = *(addr2 + k);

		// 生成校验数据B
		unsigned char B = 0;
		for (int i = 0; i < 8; i++)
		{
			B <<= 1;
			B += (__builtin_popcount(M & G[i]) % 2);
		}

		// 校验子
		unsigned char S = A ^ B;

		if(S==0)
		{
			return;
		}

		// 错误位置
		// 1位错误
		for (int j = 0; j < 8 * 2; j++)
		{
			if (S == H[j])
			{
				if (j < 8)
				{
					M ^= (1 << (7 - j));
					*(addr1 + k) = M;
				}
				else
				{
					A ^= (1 << (15 - j));
					*(addr2 + k) = A;
				}
				log_error(WB1);
				return;
			}
		}


		// 2位错误
		for (int i = 0; i < 8 * 2; i++)
		{
			for (int j = i + 1; j < 8 * 2; j++)
			{
				if (S == (H[i] ^ H[j]))
				{
					if (i < 8 && j < 8)
					{
						M ^= ((1 << (7 - i)) | (1 << (7 - j)));
						*(addr1 + k) = M;
					}
					else if (i >= 8 && j >= 8)
					{
						A ^= ((1 << (15 - i)) | (1 << (15 - j)));
						*(addr2 + k) = A;
					}
					else
					{
						M ^= (1 << (7 - i));
						A ^= (1 << (15 - j));
						*(addr1 + k) = M;
						*(addr2 + k) = A;
					}
					log_error(WB2);
					return;
				}
			}
		}
        log_error(WBn);
	}
}

void log_error(int error_code)
{
    //printf("error:%d\n",error_code);
}
