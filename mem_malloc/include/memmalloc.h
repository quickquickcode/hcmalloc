/**************************************************/
/*	分配函数头文件
/**************************************************/

#ifndef MEMMALLOC_H_
#define MEMMALLOC_H_

/************************************************/
/* include
/************************************************/

#ifdef __cplusplus
extern "c" {
#endif

#include <stddef.h>     // size_t
#include <stdbool.h>    // bool
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h> 
#include "list.h"

#define PAGE_SIZE 4096          //页大小
#define SPAN_SIZE (PAGE_SIZE+sizeof(l_span))  //span的总大小

#define POOL_SIZE (1024*1024*1024)      //初始主堆的大小

#define SPAN_CLASS_NUM 256      //span的种类
#define BLOCK_CLASS_NUM 257     //block的种类

#define SMALL_BLOCK PAGE_SIZE-sizeof(l_span)     //小内存的界限
#define LARGE_BLOCK 256*PAGE_SIZE //大内存的界限

#define SMALL_GAP 16       //小内存的跨度

#define PAGE_MAP_NUM POOL_SIZE/PAGE_SIZE  //span映射函数的数量

#define BLOCK_NUM_LIMIT 20  //线程每个小内存链表所持有小内存块的上限

#define POOL_START ((void *)((0x600000000000/PAGE_SIZE+1)*PAGE_SIZE))   //全局堆的开始地址

//#define S_POOL_NUM 15   //小内存堆上限

#define INIT_S_POOL_SPAN_NUM 256*32  //小内存分配页数量
#define INIT_S_POOL_SPAN (PAGE_SIZE*INIT_S_POOL_SPAN_NUM)   //小内存分配初始大小

#define INIT_HEAD_SPAN_NUM 20 //初始化分配给结构体堆的span的数量






typedef enum{
    UNINIT,
    INITED
}thread_state_t;

typedef struct m_pool_m m_pool;
typedef struct s_pool_m s_pool;
typedef struct l_cache_m l_cache;
typedef struct l_span_m l_span;
typedef struct m_mmap_m m_mmap;
typedef struct m_free_s_pool_list_m free_s_pool_list_m;
typedef struct m_head_pool_m head_pool_m;



//小内存堆结构体
struct s_pool_m{
    void * pool_free_start; //小内存堆空闲内存开始地址
    void * pool_end;        //小内存堆空闲内存的结束地址
    pthread_mutex_t lock;
    list_head span_free_list[BLOCK_CLASS_NUM];  //空闲的span数组链表
    list_head s_pool_list;  //小内存堆链表节点
    int span_num;
    char inited;    //0表示已经分配过内存 1表示未分配过内存
};

//线程缓存结构体
struct l_cache_m{
    list_head free_list[BLOCK_CLASS_NUM];   //小内存块的数组链表
    char block_num[BLOCK_CLASS_NUM];    //每个链表的小内存块的数量
};

//span结构体
struct l_span_m{
    int page_num;
    int total_block_num;        //总的数据块数量
    int free_block_num;         //总的空闲块数量
    void *span_free_start;
    list_head block_free_list;  //span的空闲块列表
    list_head span_free_list;   //span链表的节点
    int block_index;            //-1表示分配大内存
    s_pool *spool;              //所属的小内存堆
};

//结构体堆
struct m_head_pool_m{
    pthread_mutex_t lock;
    void *pool_start;   //内存池的开始地址
    void *free_start;   //空闲内存的开始地址
    void *pool_end;     //空闲内存的结束地址
};
//全局堆结构体
struct m_pool_m{
    pthread_mutex_t lock;
    void *pool_start;   //全局堆的开始地址
    void *free_start;   //空闲内存的开始地址
    void *pool_end;     //空闲内存的结束地址
    list_head span_free_list[SPAN_CLASS_NUM];   //全局堆的span数组链表
    l_span *span_map[PAGE_MAP_NUM*1000];    //内存地址和span结构体的映射数组，通过该数组可以找到对应地址的span的结构体
};

//小内存堆链表
struct m_free_s_pool_list_m{
    list_head pool_free_list;
};

//执行shell命令
void exeCMD(const char *cmd, char *result);

//检查是否初始化
static void check_init();

//全局初始化函数
static void global_init();

//线程初始化函数
static void thread_init();

//全局堆的初始化
static void m_pool_init();

//小内存堆的初始化
static void s_pool_init(s_pool *spool);

//小内存堆链表初始化
static void s_pool_list_init();

//block映射函数的初始化
static void maps_init();

//获取block对应的索引
static int size_cls(size_t size);

//span初始化函数
static void span_init(l_span *span,int index);

//数据块初始函数
void data_block_init(void *ptr);

//结构体堆初始化
static void m_head_pool_init();

//全局堆增长
static void mpool_grow();

//结构体堆增长
static void head_pool_grow();

//获取小内存堆结构体
static s_pool *s_pool_head_get();

//内存分配函数
void *mem_malloc(size_t size);

//小内存分配函数
static void *small_malloc(int index);

//大内存分配函数
static void *large_malloc(int index);

//超大内存分配函数
static void *huge_malloc(size_t size);

//切分span函数，将大块span切分成小块span
static l_span *span_split(l_span *span,int index);

//计算span的索引，凭借索引和映射数组找到span的数据头结构
int cal_span_index(void *free_start,void *ptr);

//从全局堆中获取span放入小内存堆中
static l_span *span_get_from_mpool(int index);

//将span切分成block
static void span_cut_block(l_span *span,int index);

//获取span的数据头结构
static l_span *span_extract_header(void *ptr);

//从小内存堆中获取block放入线程缓存中
static void block_get_from_spool(s_pool *spool,int index);

//用来向操作系统申请大块内存
static void *page_alloc(void *pos,size_t size);

//内存释放
void *mem_free(void *ptr);

//小内存释放
static void small_block_free(l_span *span,void *ptr);

//大内存释放
static void large_block_free(l_span *span,void *ptr);

//超大内存释放
static void huge_block_free(void *ptr);

//超大内存释放
static void page_free(void *pos,size_t size);

//内存读取函数
void *MEM_Read(void *ptr);

//内存写入函数
int MEM_Write(void *ptr,void *source,size_t size);

//申请并初始化内存
void *mem_calloc(size_t num, size_t size);

//重新申请内存
void *mem_realloc(void *ptr, size_t size);

//奇校验,计算数据中“1”的个数
static int get_bit_time(void *addr,size_t len);

//奇校验
static void odd_check(void *addr,int count);


#ifdef __cplusplus
}
#endif

#endif

