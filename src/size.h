#include <stdlib.h>


#ifndef page_size
#define page_size 4096
#endif


#if page_size == 4096
#define mask 13

#elif page_size == 8192
#define mask 16


#endif