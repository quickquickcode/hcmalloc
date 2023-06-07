#ifdef LOG

#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define STACK_DEPTH 16
#define CUR_FUNC_DEPTH 3

extern char _start;

char log_file_path[256]={0};



void executeCMD(const char *cmd, char *result)
{
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

uint64_t get_e_entry()
{
	FILE *fp;
	int i;
	Elf64_Ehdr elfheader;
	if((fp =  fopen(FILE_NAME, "r")) == NULL)
        perror("");
	else
		fread(&elfheader, sizeof(Elf64_Ehdr), 1, fp);
	uint64_t offset = elfheader.e_entry;
	fclose(fp);
	return offset;
}

void **STACKCALL getEBP(void)
{
	void **ebp = NULL;
	__asm__ __volatile__("mov %%rbp, %0;\n\t"
						 : "=m"(ebp)  /* 输出 */
						 :			  /* 输入 */
						 : "memory"); /* 不受影响的寄存器 */
	return (void **)(*ebp);
}

int my_backtrace(void **buffer, int size)
{

	int frame = 0;
	void **ebp;
	void **ret = NULL;
	unsigned long long func_frame_distance = 0;
	if (buffer != NULL && size > 0)
	{
		ebp = getEBP();
		func_frame_distance = (unsigned long long)(*ebp) - (unsigned long long)ebp;
		while (ebp && frame < size && (func_frame_distance < (1ULL << 24)) // assume function ebp more than 16M
			   && (func_frame_distance > 0))
		{
			ret = ebp + 1;
			buffer[frame++] = *ret;
			ebp = (void **)(*ebp);
			func_frame_distance = (unsigned long long)(*ebp) - (unsigned long long)ebp;
		}
	}
	return frame;
}

void addr_2_line(char *result)
{
	void *start = &_start;
	uint64_t offset = get_e_entry();
	void *ofs = offset;
	void *stack[STACK_DEPTH] = {0};
	int stack_depth = 0;
	int i = 0;
	/* 获取调用栈 */
	stack_depth = my_backtrace(stack, STACK_DEPTH);
	void *func_addr = stack[CUR_FUNC_DEPTH];
	void *res = func_addr - start + ofs;
	uint64_t temp = func_addr - start + ofs;
	char cmd[1024];
	char *d1 = "addr2line -e ";
	char *d2 = " ";
	snprintf(cmd,1024,"%s%s%s%p",d1,FILE_NAME,d2,temp);
	executeCMD(cmd, result);
}

void output_log_info(void *addr, size_t size,char *temp)
{
	int pid=gettid();
	char ans[1024];
	char *d1="THREAD ID:";
	char *d2="   ";
	char *d3="";
	char res[1024];
	addr_2_line(res);
	if(size!=-1){
		d3 = "   size:";
		snprintf(ans,1024,"%s%d%s%s%s%p%s%d\n",d1,pid,d2,res,temp,addr,d3,size);
	}else{
		snprintf(ans,1024,"%s%d%s%s%s%p\n",d1,pid,d2,res,temp,addr);
	}
	if(log_file_path[0]==0){
		 create_file();
	}
	w_file(log_file_path,ans);
}


void w_file(char *filepath, char *log_info)
{
	FILE *fp = NULL;
	if((fp = fopen(log_file_path, "a+")) == NULL)
        perror("");
	else
		fputs(log_info, fp);
	fclose(fp);
}

void create_file()
{	
	FILE *fp = NULL;
    time_t timep;
    time(&timep);//获取从1970至今过了多少秒，存入time_t类型的timep
    strftime(log_file_path, sizeof(log_file_path), "../log/%Y.%m.%d %H-%M-%S.txt",localtime(&timep) ); //最后一个参数是用localtime将秒数转化为struct tm结构体
    if((fp = fopen(log_file_path, "a+")) == NULL)
        perror("");
    fclose(fp);
	
}

#endif



