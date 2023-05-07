struct thread_cache
{
    struct thread_cache* next;//????????????
    struct thread_cache* pre;

    int size;

    freelist free_[];

};
