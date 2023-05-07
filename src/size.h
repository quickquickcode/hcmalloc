#include <stdlib.h>

//如果没有在GCC中定义页面大小 默认定义为4k
#ifndef page_size
#define page_size 4096
#endif

//根据不同大小定义需要的宏

#if page_size == 4096
    //定义掩码
    #define mask 13
    //定义这个页面大小下有多少个不同的大小    数字不准确 需要修改
    #define size_num 2
    //定义一个由大小获取下标的宏
    #define getidx(size) \
          
#elif page_size == 8192
    #define mask 14


#endif