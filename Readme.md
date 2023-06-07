# hcmalloc

赛题选择：[proj28-3RMM](https://github.com/oscomp/proj28-3RMM)

队伍名称：勇敢冲就队

队员姓名：黄棋彬，朱家明，谭忻

指导教师：李超

学校名称：广州大学

## 项目描述

​	hcmalloc是一个运行在用户态，可靠、健壮、实时的C语言内存分配器，具有抗单粒子翻转功能。

​	在hcmalloc中，具有线程缓存和中央内存池两级缓存，有从大到小book，page，block三层内存角色，以帮助快速分配释放内存。

​	详细技术实现请参照：`./doc/技术文档.md`

## 使用说明

请使用以下命令编译动态连接库

```makefile
cd ./src
make v1
```

hcmalloc可以选择不同的锁类型，是否超额分配等多种功能

```makefile
# 标准版本(mutex锁)
make v1

# 使用自旋锁
make v2

# 打开超额分配
make v3

# 打开分配内存地址检查
make v4
```

使用案例

```c
#include "hcmalloc.h"

char data1[10]="0123456789abcde";
char data2[10]="";

int main()
{
    // 申请0x10大小内存
	void* a=hcmalloc(0x10);
    
    // 更改内存大小
    a=hcrealloc(a,0x20);
   	
    // 申请内存并初始化
    void* b=hccalloc(0x10);
    
    //  申请一块抗单粒子翻转的内存
    void* c=safe_malloc(0x10);
    
    // 向申请的内存写入长度0x10的数据
    safe_write(c,data1,0x10);
    
    // 从申请的内存读取长度0x10的数据
    safe_read(data2,c,0x10);
    
    // 释放内存
    hcfree(a);
    hcfree(b);
    hcfree(c);
    
    return 0;
}
```



## 测试

功能性测试执行文件在test文件夹下

以下是测试命令，输出test done！则测试成功

```bash
cd ./test
sh run_tests.sh
```

性能测试执行文件在benchmark目录下，测试结果保存在images

以下是测试命令

```bash
cd ./benchmark
sh run_tests.sh
```



## 提交仓库目录和文件描述

* benchmark
     * images:存放生成的测试结果图片
     * test_code:
          * exp_*.c：测试程序源码
     * exp_*.sh:测试脚本
     * static_analy.py:生成测试图片的python脚本
* doc：项目文档目录
* src：
     * hcmalloc.c：hcmalloc的源文件
     * hcmalloc.h：hcmalloc的头文件
     * error.h：记录错误信息的头文件
     * Makefile build文件
* test：
     * *.c：测试源文件
     * Makefile build文件
* memmalloc:上届优秀内存分配器源代码
