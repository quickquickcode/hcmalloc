#include <pthread.h>


/*
  -----------------------  define -----------------------
*/

#ifdef spin 
    #define lock_t pthread_spinlock_t
    #define lock_init(a) pthread_spin_init(a,PTHREAD_PROCESS_PRIVATE);
    #define lock(a) pthread_spin_lock(a)
    #define trylock(a) pthread_spin_trylock(a)
    #define unlock(a) pthread_spin_unlock(a)

#else
    #define lock_t pthread_mutex_t
    #define lock_init(a) pthread_mutex_init(a,PTHREAD_PROCESS_PRIVATE);
    #define lock(a) pthread_mutex_lock(a)
    #define trylock(a) pthread_mutex_trylock(a)
    #define unlock(a) pthread_mutex_unlock(a)
#endif

//#define Over_alloc
#ifdef Over_alloc
#endif




// page和book
#define PAGE_SIZE 8192      // 8K
#define PAGE_SHIFT 13
#define PAGE_MASK PAGE_SIZE-1
#define BOOK_SIZE 0x400000  // 4MB
#define BOOK_SHIFT 22
#define BOOK_MASK BOOK_SIZE-1
#define SIZE_CLASS_NUM 86   // 有多少种size

#define BLOCK_MAX_SIZE  0x40000
#define SPAN_MAX_SIZE   0x100000
#define LAST_HEADER_OFFSET PAGE_SIZE-16

#define kAlignment 16
#define kMaxSmallSize 1024

// block和idx
#define size2idx(size) class_size_id(size)
#define idx2size(idx)  idx_to_size[idx]
#define get_page_num(idx) idx_to_pages[idx]

// header和page相互转换
#define page2header(addr) ((header*)\
        (((uintptr_t)(addr) & ~((1 << BOOK_SHIFT) - 1)) + \
        ((((uintptr_t)(addr) >> PAGE_SHIFT) & ((1 << (BOOK_SHIFT - PAGE_SHIFT)) - 1)) << 4)))


#define header2page(addr) ((page*) \
        ((((uintptr_t)(addr) & ((1 << PAGE_SHIFT) - 1)) << (PAGE_SHIFT-4)) + \
        ((uintptr_t)(addr) & (~((1 << BOOK_SHIFT) - 1)))))


// 获取header的相关信息
#define get_header_ishead(addr) \
        page2header(addr)->hup&(1<<2)

#define get_header_inuse(addr)  \
        page2header(addr).hup&(1<<1)

#define get_header_pinuse(addr)  \
        page2header(addr).hup&(1)
        



// 获取__span_bin中的span
#define get_next_book_span()
#define get_next_span()

// 抗单字节翻转的bool
#define get_bool(a)
#define set_bool(a, b)



// 获取线程缓存的第一个block
#define block_bin(idx) tcache._block_bin[idx]



// 返回最高位
#define max_bit(x) 31 - __builtin_clz(x)


/*
  -----------------------  typedef -----------------------
*/

typedef struct thread_cache_s thread_cache;
typedef struct header_s header;
typedef struct page_s page;
typedef struct block_s block;
typedef struct bin_s bin;


/*
  -----------------------  struct -----------------------
*/

struct bin_s
{
  bin* next;
  bin* prev;
};


/*

*/
struct block_s
{
  struct block_s* next;
  struct block_s* prev;
};


/*
  线程缓存
*/
struct  thread_cache_s
{
  block _block_bin[SIZE_CLASS_NUM];
  block* _need_free_list;
  int _span_num[SIZE_CLASS_NUM];
  page* _last_book;

  int _need_free_num;
  unsigned char _above_times[SIZE_CLASS_NUM];
  bin _span_bin[9];
  page* _book_list;
  unsigned int _book_num;
};

/*
  一个header为16个字节
*/
struct header_s
{
  unsigned char block_size_idx;
  unsigned char hup;  
  unsigned short  span_len;
  unsigned short prev_span_len;
  unsigned short use_num;
  char a[8];
};

/*
  page_s(page)数据结构只有在该page为header_page和空闲时有效
  top两个指针为复用结构
    在header_page中两个指针:
        指向下一个book
        校验码
    在free_page中两个指针分别链接向:
        下一个同等大小free_page
        上一个同等大小free_page
  page_header在header page中有效 
*/
struct page_s
{
  page* next;
  page* prev;
  header page_header[511];
};




/*
  -----------------------  函数头 -----------------------
*/


// 检查初始化
static void *check_init();

// 检查线程初始化
static void global_init();

// 检查全局初始化
static void thread_init();

// 向系统申请空间
static inline void add_book(int num);

// 初始化一个book
static inline void *book_init(page* victim);

// 向中央申请book并初始化
static inline void trans_book(int num);

// 向中央申请book
static inline int get_book(int num);

// 向中央归还book
static inline void *ret_book(page* book_addr);

// mallloc 申请内存
void *hcmalloc(size_t size);

// 向线程缓存申请小内存
static void* small_malloc(size_t size);

// 向线程缓存申请大内存
static void* big_malloc(size_t size);

// 向系统申请超大内存
static void* huge_malloc(size_t size);

// free 释放内存
void hcfree(void *addr);

// 释放小内存块
static inline void small_free(void *addr,int idx);

// 释放大内存
static inline void big_free(header* p);

// 释放超大内存
static inline void huge_free(void* addr);

// 重新分配addr对应的块为size大小的块
void* hcrealloc(void *addr,size_t size);

// 申请内存并初始化
void* hccalloc(size_t size);

// 申请一个可以抗单粒子翻转的内存空间
void* safe_malloc(size_t size);

// 向内存写入一段size大小的数据
void* safe_write(void* ptr,void* data,size_t size);

// 纠正内存并读取size大小的数据
void* safe_read(void* w_addr,void* ptr,size_t size);

// 销毁线程缓存
void tcache_destroy(void* arg);

// 将span放入_span_bin 
static inline void link_span(bin* p);

// 将span从_span_bin中取出
static inline void unlink_span(page* p);

// 检查是否有block
static inline int check_block(int idx);

// 从block_bin中取出一个
static inline block* pop_block(int idx);

// 将block放入block_bin中
static inline void push_block2bin(block* p,int idx);

// 将block从链中取出
static inline void unlink_block(block* p);

// 获得span第一个page的header地址
static inline header* get_span_header(void *addr);



// 抗单字节翻转
// 将以addr1为起始size大小的数据校验码记录在addr2位置
static void note_data(char* addr1,char* addr2, int size);

// 通过addr2位置检查addr1位置的数据
static void check_data(char* addr1,char* addr2, int size);

// 将大小转化为对应的下标
static int class_size_id(int size);