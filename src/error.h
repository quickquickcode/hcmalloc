#include <pthread.h>
#include <time.h>



struct error_s
{
    pthread_t id;
    int error_num;
    clock_t time;
};


#define BAD_PTR 1
#define ATTACK 2
#define WB1 3
#define WB2 4
#define WBn 5





void log_error(int error_code);