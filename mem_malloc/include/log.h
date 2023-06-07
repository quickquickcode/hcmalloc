/**************************************************/
/*	分配函数头文件
/**************************************************/

#ifndef LOG_H_
#define LOG_H_

/************************************************/
/* include
/************************************************/

#ifdef __cplusplus
extern "c" {
#endif

#include <stdio.h>
#include "/usr/include/elf.h"

#define STACKCALL __attribute__((regparm(1),noinline))

//执行shell
void executeCMD(const char *cmd, char *result);

//获取.text偏移
uint64_t get_e_entry();

//获取ebp寄存器内容
void ** STACKCALL getEBP(void);

//获取函数调用栈
int my_backtrace(void **buffer,int size);

//将地址转换为代码行号
void addr_2_line(char *res);

//输出日志信息
void output_log_info(void *addr,size_t size,char *temp);

//创建文件
void create_file();

//写入日志信息到文件
void w_file(char *filepath, char *log_info);

#ifdef __cplusplus
}
#endif

#endif


