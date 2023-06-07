#include "memmalloc.h"

#ifdef LOG
#include "log.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "pthread.h"

pthread_once_t init_once = PTHREAD_ONCE_INIT;
__thread thread_state_t thread_state = UNINIT; //线程初始化状态
thread_state_t global_state = UNINIT;          //全局初始化状态
__thread l_cache lcache;                       //线程缓存
m_pool mpool;                                  //全局堆
head_pool_m head_pool;                         //结构体堆

int s_cls_size[BLOCK_CLASS_NUM]; //小内存块类和大小的映射数组
int s_sizemap[BLOCK_CLASS_NUM];  //小内存块大小和类的映射数组，通过它可以快速找到小内存块大小属于哪个类

free_s_pool_list_m free_s_pool_list; //小内存堆链表

int s_pool_num_s ;
int bn = 15;

void exeCMD(const char *cmd, char *result){
    result[0] = '\0';
	char buf_ps[1024];
	char ps[1024] = {0};
	FILE *ptr;
	strcpy(ps, cmd);
	if ((ptr = popen(ps, "r")) != NULL)
	{
		fgets(buf_ps, 1024, ptr);
		strcpy(result, buf_ps);
		pclose(ptr);
		ptr = NULL;
	}
	else
	{
		printf("popen %s error\n", ps);
	}
	result[strlen(result) - 1] = '\0';
}

static void check_init()
{
    if (thread_state == UNINIT)
    {
        if (thread_state == UNINIT)
        {
            pthread_once(&init_once, global_init);
        }
        thread_init();
        thread_state = INITED;
    }
}

static void global_init()
{

    #ifdef SPOOL_NUM
    s_pool_num_s = SPOOL_NUM;
    #else
    char res[128];
    char *cmd = "cat /proc/cpuinfo | grep 'processor' | wc -l";
    exeCMD(cmd,res);
    s_pool_num_s = atoi(res);
    #endif
    //初始化全局堆
    m_pool_init();
    //初始化结构体堆
    m_head_pool_init();
    //初始化小内存链表
    s_pool_list_init();
    //初始化映射数组
    maps_init();
    global_state = INITED;
    s_pool_num_s = 1;
}

static void thread_init()
{
    //初始化线程缓存的空闲数据块的数组链表
    for (int i = 0; i < BLOCK_CLASS_NUM; i++)
    {
        INIT_LIST_HEAD(&lcache.free_list[i]);
        lcache.block_num[i] = 0;
    }
}

static void m_pool_init()
{
    if (pthread_mutex_init(&mpool.lock, NULL) < 0)
    {
        fprintf(stderr, "fatal error:pthread_mutex_init failed\n");
        exit(-1);
    }
    //主堆调用mmap申请1GB内存
    void *addr = page_alloc(POOL_START, POOL_SIZE);
    mpool.pool_start = (void *)(((u_int64_t)addr + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE);
    mpool.pool_end = addr + POOL_SIZE;
    mpool.free_start = mpool.pool_start;
    //初始化span的数组链表
    for (int i = 0; i < SPAN_CLASS_NUM; i++)
    {
        INIT_LIST_HEAD(&mpool.span_free_list[i]);
    }
}

//初始时分配给小内存堆64MB内存
static void s_pool_init(s_pool *spool)
{
    //从主堆中获取初始内存
    pthread_mutex_lock(&mpool.lock);
    if (mpool.free_start + INIT_S_POOL_SPAN <= mpool.pool_end)
    {
        spool->pool_free_start = mpool.free_start;
        spool->pool_end = spool->pool_free_start + INIT_S_POOL_SPAN;
        mpool.free_start += INIT_S_POOL_SPAN;
    }
    else
    {
        mpool_grow();
        spool->pool_free_start = mpool.free_start;
        spool->pool_end = spool->pool_free_start + INIT_S_POOL_SPAN;
        mpool.free_start += INIT_S_POOL_SPAN;
    }
    pthread_mutex_unlock(&mpool.lock);
    spool->span_num = INIT_S_POOL_SPAN_NUM;
    //初始化span的数组链表
    for (int i = 0; i < BLOCK_CLASS_NUM; i++)
    {
        INIT_LIST_HEAD(&spool->span_free_list[i]);
    }
    spool->inited = 1;
}

static void s_pool_list_init()
{

    s_pool *spool;
    INIT_LIST_HEAD(&free_s_pool_list.pool_free_list);
    //获取小内存堆数据结构体
    spool = s_pool_head_get();
    //初始化小内存堆
    s_pool_init(spool);
    if (pthread_mutex_init(&spool->lock, NULL) < 0)
    {
        fprintf(stderr, "fatal error:pthread_mutex_init failed\n");
        exit(-1);
    }
    // spool挂到小内存链表中
    list_add(&spool->s_pool_list, &free_s_pool_list.pool_free_list);
    //创建其他小内存池
    for (int i = 2; i <= s_pool_num_s; i++)
    {
        spool = s_pool_head_get();
        spool->inited = 0;
        if (pthread_mutex_init(&mpool.lock, NULL) < 0)
        {
            fprintf(stderr, "fatal error:pthread_mutex_init failed\n");
            exit(-1);
        }
        list_add(&spool->s_pool_list, &free_s_pool_list.pool_free_list);
    }
};

static void maps_init()
{
    int size;
    int class;
    for (size = 8, class = 0; size <= SMALL_BLOCK; size += SMALL_GAP, class ++)
    {
        s_cls_size[class] = size;
    }
    int cur_class = 0;
    int cur_size = 0;
    for (cur_size = 8; cur_size <= SMALL_BLOCK; cur_size += SMALL_GAP)
    {

        if (cur_size > s_cls_size[cur_class])
        {
            cur_class++;
        }
        s_sizemap[(cur_size - 1) >> 4] = cur_class;
    }
}

static int size_cls(size_t size)
{
    int index = s_sizemap[(size - 1) >> 4];
    return index;
}

static void span_init(l_span *span, int index)
{
    span->span_free_start = (void *)span + sizeof(l_span);
    span->page_num = index + 1;
    span->block_index = -1;
    INIT_LIST_HEAD(&span->block_free_list);
    INIT_LIST_HEAD(&span->span_free_list);
}

static void m_head_pool_init()
{
    if (pthread_mutex_init(&head_pool.lock, NULL) < 0)
    {
        fprintf(stderr, "fatal error:pthread_mutex_init failed\n");
        exit(-1);
    }
    pthread_mutex_lock(&mpool.lock);
    //从全局堆获取内存
    head_pool.pool_start = mpool.free_start;
    head_pool_grow();
    pthread_mutex_unlock(&mpool.lock);
}

static void head_pool_grow()
{
    if (mpool.free_start + PAGE_SIZE * INIT_HEAD_SPAN_NUM <= mpool.pool_end)
    {
        head_pool.free_start = mpool.free_start;
        head_pool.pool_end = head_pool.free_start + PAGE_SIZE * INIT_HEAD_SPAN_NUM;
        mpool.free_start += PAGE_SIZE * INIT_HEAD_SPAN_NUM;
    }
    else
    {
        mpool_grow();
        head_pool.free_start = mpool.free_start;
        head_pool.pool_end = head_pool.free_start + PAGE_SIZE * INIT_HEAD_SPAN_NUM;
        mpool.free_start += PAGE_SIZE * INIT_HEAD_SPAN_NUM;
    }
}

static void mpool_grow()
{
    void *addr = page_alloc(mpool.pool_end, POOL_SIZE);
    if (addr < 0)
    {
        fprintf(stderr, "page_alloc failed\n");
        exit(-1);
    }
    mpool.free_start = (void *)(((u_int64_t)addr + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE);
    mpool.pool_end = mpool.pool_end + POOL_SIZE;
}

static s_pool *s_pool_head_get()
{
    s_pool *spool;
    pthread_mutex_lock(&head_pool.lock);
    if (head_pool.free_start + sizeof(s_pool) <= head_pool.pool_end)
    {
        spool = head_pool.free_start;
        head_pool.free_start += sizeof(s_pool);
    }
    else
    {
        head_pool_grow();
        spool = head_pool.free_start;
        head_pool.free_start += sizeof(s_pool);
    }
    pthread_mutex_unlock(&head_pool.lock);
    return spool;
}

void *mem_malloc(size_t size)
{
    void *addr = NULL;
    if (size == 0)
    {
        return addr;
    }
    check_init();
#ifdef SEU
    size = (size + 1) * 2;
#elif TMR
    size = size * 3;
#endif
    if (size <= SMALL_BLOCK)
    {
        //小内存分配
        int index = size_cls(size);
        addr = small_malloc(index);
    }
    else if (size <= LARGE_BLOCK)
    {
        //大内存分配
        int index = (size + sizeof(l_span)) / PAGE_SIZE;
        addr = large_malloc(index);
    }
    else
    {
        //超大内存分配
        addr = huge_malloc(size);
    }

#ifdef LOG
    char *str = "   malloc   start:";
    output_log_info(addr, size, str);
#endif

    return addr;
}

static void *small_malloc(int index)
{
    void *addr = NULL;
    //尝试从local_chche中获取对应内存块
    if (list_empty(&lcache.free_list[index]))
    {
        //查找空闲小内存池链表
        bool flag = false;                                           // false表示没有可用空闲小内存池
        list_head *list_node = free_s_pool_list.pool_free_list.next; //获取小内存堆节点
        for (int i = 1; i <= s_pool_num_s; i++)
        {
            s_pool *spool = list_entry(list_node, s_pool, s_pool_list);
            if (!pthread_mutex_trylock(&spool->lock))
            {
                if (spool->inited == 0)
                {
                    //小内存堆没有分配过空闲内存，给小内存堆分配内存
                    s_pool_init(spool);
                }
                //从小内存堆获取空闲数据块

                block_get_from_spool(spool, index);
                flag = true;
                pthread_mutex_unlock(&spool->lock);
                break;
            }
            list_node = list_node->next;
        }
        //小内存堆链表中的小内存堆都被占用，默认等待链表下的第一个小内存堆
        if (flag == false)
        {
            s_pool *spool = list_entry(free_s_pool_list.pool_free_list.next, s_pool, s_pool_list);
            pthread_mutex_lock(&spool->lock);
            //从小内存堆中获取空闲数据块
            block_get_from_spool(spool, index);
            pthread_mutex_unlock(&spool->lock);
        }
    }
    //从线程缓存中获取内存块
    addr = lcache.free_list[index].next;
    l_span *span = span_extract_header(addr);
    list_del((list_head *)addr);
    lcache.block_num[index]--;
    return addr;
};

static void *large_malloc(int index)
{
    pthread_mutex_lock(&mpool.lock);
    //从全局堆中获取span
    l_span *span = span_get_from_mpool(index);
    pthread_mutex_unlock(&mpool.lock);
    return span->span_free_start;
}

static void *huge_malloc(size_t size)
{
    void *addr;
    size_t tot_size = size + sizeof(u_int64_t);
    //调用mmap获取内存
    addr = page_alloc(NULL, tot_size);
    *(u_int64_t *)addr = size;
    return addr + sizeof(u_int64_t);
}

static l_span *span_split(l_span *span, int index)
{
    int page_num = index + 1;
    int diff = span->page_num - page_num;
    l_span *s;
    if (diff > 0)
    {
        span_init(span, index);
        l_span *s = (void *)span + PAGE_SIZE * (page_num);
        span_init(s, diff - 1);
        for (int i = 0; i < diff; i++)
        {
            int ind = cal_span_index(mpool.pool_start, (void *)s + i * PAGE_SIZE);
            mpool.span_map[ind] = s;
        }
        list_add(&s->span_free_list, &mpool.span_free_list[diff - 1]);
    }
    return span;
}

int cal_span_index(void *pool_start, void *ptr)
{
    int index = (ptr - pool_start) / PAGE_SIZE;
    return index;
}

static l_span *span_get_from_mpool(int index)
{
    l_span *span = NULL;
    bool flag = false;
    //先从堆中划分
    if ((mpool.free_start + (index + 1) * PAGE_SIZE) <= mpool.pool_end)
    {
        span = mpool.free_start;
        mpool.free_start = mpool.free_start + (index + 1) * PAGE_SIZE;
        //初始化span
        span_init(span, index);
        //建立基础span的索引和基础span所属大span的结构体地址的映射关系
        for (int i = 0; i <= index; i++)
        {
            int ind = cal_span_index(mpool.pool_start, (void *)span + i * PAGE_SIZE);
            mpool.span_map[ind] = span;
        }
        flag = true;
    }
    else
    {
        //主堆中没有为分配过的内存，就从span的数组链表中去找
        for (int i = index; i < SPAN_CLASS_NUM; i++)
        {
            if (!list_empty(&mpool.span_free_list[i]))
            {
                span = list_entry(mpool.span_free_list[i].next, l_span, span_free_list);
                list_del(mpool.span_free_list[i].next);
                //切分span
                span = span_split(span, index);
                flag = true;
                break;
            }
        }
    }
    //主堆中既没有空闲内存也没有满足要求的span，扩展主堆
    if (flag == false)
    {
        //扩展堆
        mpool_grow();
        //继续从堆中划分
        span = mpool.free_start;
        mpool.free_start = mpool.free_start + (index + 1) * PAGE_SIZE;
        span_init(span, index);
        for (int i = 0; i <= index; i++)
        {
            int ind = cal_span_index(mpool.pool_start, (void *)span + i * PAGE_SIZE);
            mpool.span_map[ind] = span;
        }
    }
    return span;
}

static void span_cut_block(l_span *span, int index)
{
    int count = ((span->page_num) * PAGE_SIZE - sizeof(l_span)) / ((index + 1) * SMALL_GAP);
    span->total_block_num = count;
    span->free_block_num = count;
    span->block_index = index;
    list_head *block;
    for (int i = 0; i < count; i++)
    {
        block = span->span_free_start;
        list_add(block, &span->block_free_list);
        span->span_free_start = span->span_free_start + SMALL_GAP * (index + 1);
    }
}

static l_span *span_extract_header(void *ptr)
{
    if (ptr < mpool.pool_start || ptr > mpool.pool_end)
    {
        return NULL;
    }
    //获取ptr对应的span
    int index = cal_span_index(mpool.pool_start, ptr);
    l_span *span = mpool.span_map[index];
    return span;
}

static void block_get_from_spool(s_pool *spool, int index)
{
    l_span *span = NULL;
    if (list_empty(&spool->span_free_list[index]))
    {
        //从小内存池中未分配的内存块开始分配
        if (spool->pool_free_start + PAGE_SIZE <= spool->pool_end)
        {
            span = spool->pool_free_start;
            spool->pool_free_start = spool->pool_free_start + PAGE_SIZE;
            //初始化span
            span_init(span, 0);
            int ind = cal_span_index(mpool.pool_start, (void *)span);
            mpool.span_map[ind] = span;
        }
        else
        {
            //尝试从主堆获取
            pthread_mutex_lock(&mpool.lock);
            span = span_get_from_mpool(0);
            pthread_mutex_unlock(&mpool.lock);
        }
        span->spool = spool;
        //将span挂到链表下面,并切分
        list_add(&span->span_free_list, &spool->span_free_list[index]);
        span_cut_block(span, index);
    }
    span = list_entry(spool->span_free_list[index].next, l_span, span_free_list);
    //从链表下面摘取10个块放入线程缓存
    int count = bn;
    if (span->free_block_num < bn)
    {
        count = span->free_block_num;
    }
    list_head *block;
    for (int i = 1; i <= count; i++)
    {
        block = span->block_free_list.next;
        list_del(span->block_free_list.next);
        span->free_block_num--;
        list_add(block, &lcache.free_list[index]);
        lcache.block_num[index]++;
    }
    if (span->free_block_num == 0)
    {
        list_del(&span->span_free_list);
    }
}

void *mem_free(void *ptr)
{
    if (ptr == NULL)
    {
        return NULL;
    }
    //获取内存所属的span的结构体
    l_span *span = span_extract_header(ptr);
    if (span == NULL)
    {
        //大内存
        huge_block_free(ptr);
    }
    else if (span->page_num == 1)
    {
        //小内存释放
        // pthread_mutex_lock(&span->spool->lock);
        small_block_free(span, ptr);
        // pthread_mutex_unlock(&span->spool->lock);
    }
    else if (span->page_num <= SPAN_CLASS_NUM)
    {
        //大内存释放
        large_block_free(span, ptr);
    }
#ifdef LOG
    char *str = "   free     start:";
    int size = -1;
    output_log_info(ptr, size, str);
#endif
}

static void *page_alloc(void *pos, size_t size)
{
    return mmap(pos, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

static void page_free(void *pos, size_t size)
{
    munmap(pos, size);
}

static void small_block_free(l_span *span, void *ptr)
{
    int index = span->block_index;
    //首先检查线程缓存对应的块列表是否超出空闲内存块的数量限制
    if (lcache.block_num[index] < bn * 2)
    {
        //没有超出空闲块限制，可以放入
        list_add((list_head *)ptr, &lcache.free_list[index]);
        lcache.block_num[index]++;
        return;
    }
    pthread_mutex_lock(&span->spool->lock);
    list_add((list_head *)ptr, &span->block_free_list);
    span->free_block_num++;
    if (span->free_block_num == 1)
    {
        //将span放回小内存堆
        list_add(&span->span_free_list, &span->spool->span_free_list[index]);
    }
    if (span->free_block_num == span->total_block_num)
    {
        //放回大内存堆
        pthread_mutex_lock(&mpool.lock);
        list_add(&span->span_free_list, &mpool.span_free_list[span->page_num - 1]);
        INIT_LIST_HEAD(&span->block_free_list);
        span->span_free_start = (void *)span + sizeof(l_span);
        pthread_mutex_unlock(&mpool.lock);
    }
    pthread_mutex_lock(&span->spool->lock);
}

static void large_block_free(l_span *span, void *ptr)
{
    pthread_mutex_lock(&mpool.lock);
    //将释放的span重现挂到主堆对应的span链表中
    list_add(&span->span_free_list, &mpool.span_free_list[span->page_num - 1]);
    pthread_mutex_unlock(&mpool.lock);
}

static void huge_block_free(void *ptr)
{
    void *temp = ptr - sizeof(u_int64_t);
    //调用unmmap释放内存
    u_int64_t size = *(u_int64_t *)temp;
    page_free(temp, size + sizeof(u_int64_t));
}

#ifdef TMR

//参考上届优秀作品trmalloc
void *MEM_Read(void *ptr)
{
    l_span *span = span_extract_header(ptr);
    int bsize;
    if (span != NULL)
    {
        if (span->block_index != -1)
        {
            bsize = (span->block_index + 1) * SMALL_GAP / 3;
        }
        else
        {
            bsize = ((span->page_num) * PAGE_SIZE - sizeof(l_span)) / 3;
        }
    }
    //获得第一冗余块的开始写入地址
    void *f_addr = ptr + bsize;
    //获得第二个冗余块的开始写入地址
    void *s_addr = ptr + bsize + bsize;
    // compare数组，初始化为0，表示3次对比的结果，即1与2，1与3，2与3对比，为0表示两块内容相同
    int compare[3] = {0, 0, 0};
    if (memcmp(ptr, f_addr, bsize))
    { // 1与2对比，内容不同置为1
        compare[0] = 1;
    }
    if (memcmp(ptr, s_addr, bsize))
    { // 1与3对比，内容不同置为1
        compare[1] = 1;
    }
    if (memcmp(f_addr, s_addr, bsize))
    { // 2与3对比，内容不同置为1
        compare[2] = 1;
    }
    int flag = 0;   // flag为0表示三块都相同 为1表示有一块不同
    int index = -1; // index指向三次比较中两块都相同的一次比较
    for (int i = 0; i < 3; i++)
    {
        if (compare[i] == 1)
        {
            flag = 1;
        }
        if (compare[i] == 0)
        {
            index = i;
        }
    }
    // 三块都相同，不需要做恢复，直接返回ptr
    if (flag == 0)
    {
        return ptr;
    }
    // 三次比较都不同，不止一个bit翻转，出错,返回第一块
    if (index == -1)
    {
        return NULL;
    }
    // 有一个块发生了翻转
    if (index == 0)
    { // 1与2 相同，说明第3块发生翻转，用1的内容覆盖3
        memcpy(s_addr, ptr, bsize);
    }
    else if (index == 1)
    { // 1与3 相同，说明第2块发生翻转，用1的内容覆盖2
        memcpy(f_addr, ptr, bsize);
    }
    else
    { // 2与3 相同，说明第1块发生翻转，用2的内容覆盖1
        memcpy(ptr, f_addr, bsize);
    }
    return ptr;
}

int MEM_Write(void *ptr, void *source, size_t size)
{
    l_span *s = span_extract_header(ptr);
    int bsize;
    if (s != NULL)
    {
        if (s->block_index != -1)
        {
            bsize = (s->block_index + 1) * SMALL_GAP / 3;
        }
        else
        {
            bsize = ((s->page_num) * PAGE_SIZE - sizeof(l_span)) / 3;
        }
        //获得第一冗余块的开始写入地址
        void *f_addr = ptr + bsize;
        //获得第二个冗余块的开始写入地址
        void *s_addr = ptr + bsize + bsize;
        //将写入的值同时写入第一、二、三块中
        memcpy(ptr, source, size);
        memcpy(f_addr, source, size);
        memcpy(s_addr, source, size);
    }

    return 1;
}

#elif SEU

int get_bit_time(void *addr, size_t len)
{
    int count = 0;
    char *temp = (char *)addr;
    for (int i = 1; i <= len; i++)
    {
        char c = *temp;
        for (int j = 0; j < 8; j++)
        {
            count += (c >> j) & 1;
        }
        temp = temp + 1;
    }
    return count;
}

static void odd_check(void *addr, int count)
{
    char *temp = (char *)addr;
    if (count % 2 == 0)
    {
        *temp = 1;
    }
    else
    {
        temp = 0;
    }
}

void *MEM_Read(void *ptr)
{
    l_span *span = span_extract_header(ptr);
    int bsize, count1, count2;
    void *block_addr;
    void *f_addr;
    if (span != NULL)
    {
        if (span->block_index != -1)
        {
            bsize = (span->block_index + 1) * SMALL_GAP / 2;
            u_int64_t temp = ptr;
            temp = (temp % PAGE_SIZE - sizeof(l_span)) % ((span->block_index + 1) * SMALL_GAP);
            block_addr = ptr - temp;
            //获得第一冗余块的开始写入地址
            f_addr = ptr + bsize;
            count1 = get_bit_time(block_addr, bsize);
            count2 = get_bit_time(block_addr + bsize, bsize);
        }
        else
        {
            bsize = ((span->page_num) * PAGE_SIZE - sizeof(l_span)) / 2;
            block_addr = (void *)span + sizeof(l_span);
            //获得第一冗余块的开始写入地址
            f_addr = ptr + bsize;
            count1 = get_bit_time(block_addr, bsize);
            count2 = get_bit_time(block_addr + bsize, bsize);
        }
        if (count1 % 2 == 0)
        {
            memcpy(ptr, f_addr, bsize);
        }
        if (count2 % 2 == 0)
        {
            memcpy(f_addr, ptr, bsize);
        }
    }

    return ptr;
}

int MEM_Write(void *ptr, void *source, size_t size)
{
    l_span *s = span_extract_header(ptr);
    int bsize;
    void *block_addr;
    if (s != NULL)
    {
        if (s->block_index != -1)
        {

            bsize = (s->block_index + 1) * SMALL_GAP / 2;
            u_int64_t temp = ptr;
            temp = (temp % PAGE_SIZE - sizeof(l_span)) % ((s->block_index + 1) * SMALL_GAP);
            block_addr = ptr - temp;
        }
        else
        {
            bsize = ((s->page_num) * PAGE_SIZE - sizeof(l_span)) / 2;
            block_addr = (void *)s + sizeof(l_span);
        }
        void *f_addr = ptr + bsize;
        memcpy(ptr, source, size);
        memcpy(f_addr, source, size);
        int count = get_bit_time(block_addr, bsize - 1);
        odd_check(block_addr + bsize - 1, count);
        odd_check(block_addr + bsize * 2 - 1, count);
    }
    return 1;
}
#else

int MEM_Write(void *ptr, void *source, size_t size)
{
    memcpy(ptr, source, size);
    return 1;
}

void *MEM_Read(void *ptr)
{
    return ptr;
}
#endif

void data_block_init(void *ptr)
{
    l_span *s = span_extract_header(ptr);
    int bsize;
    if (s != NULL)
    {
        if (s->block_index != -1)
        {
            bsize = (s->block_index + 1) * SMALL_GAP;
        }
        else
        {
            bsize = (s->page_num) * PAGE_SIZE - sizeof(l_span);
        }
    }
    memset(ptr, 0, bsize);
}

void *mem_calloc(size_t num, size_t size)
{
    /* 在内存的动态存储区中分配num个长度为size的连续空间 */
    register void *addr;
    if (num == 0 || size == 0)
    {
        return NULL;
    }
    addr = mem_malloc(num * size);
    if (addr)
        bzero(addr, num * size);
    return addr;
}

//参考ptmalloc
void *mem_realloc(void *ptr, size_t size)
{
    void *addr = NULL;
    if (ptr == NULL)
    {
        return mem_malloc(size);
    }
    if (size == 0)
    {
        mem_free(ptr);
        return NULL;
    }
    l_span *span = span_extract_header(ptr);
    if (size <= PAGE_SIZE)
    {
        if (size <= (span->block_index + 1) * SMALL_GAP)
        {
            return ptr;
        }
        else
        {
            addr = mem_malloc(size);
            return addr;
        }
    }
    else if (size <= LARGE_BLOCK)
    {
        if (size <= (span->page_num) * PAGE_SIZE)
        {
            return ptr;
        }
    }
    else
    {
        void *temp = ptr - sizeof(u_int64_t);
        u_int64_t sizee = *(u_int64_t *)temp;
        if (size <= sizee)
        {
            return ptr;
        }
    }
    addr = mem_malloc(size);
    memcpy(ptr, addr, size);
    return addr;
}

//#endif