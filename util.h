/* util.h */

#ifndef __UTIL_H_
#define __UTIL_H_
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include "wrap/wrap.h"

struct cmd {
	char protocol[16];
	char serv_name[128]; // 服务器的FQDN名称
	char serv_port[8]; // 服务器web服务器的端口
	char file_name[512]; //对应服务器上要下载的文件的路径
	char save_name[512]; // 保存到本地的下载的文件的名字
	unsigned short thread_num; //下载所使用的线程数
};

struct task { // 用于多线程下载任务结构体
	struct sockaddr_in addr;  // server的ip和端口
	char server[32];
	char server_port[8];
	ssize_t total;   		// 文件总长度
	ssize_t start;			// 此线程开始的位置
	ssize_t end;			//此线程结束的位置
	ssize_t offset;			// 此线程完成的字节数
	char save_file[128];			//此线程要保存的文件名称
	char serv_file[1024];     // 线程要下载的文件名字 在服务器端的路径
};

// 初始化参数，如果成功把参数保存到options中，失败打印出错信息并退出
void init_args(int argc, char * argv[], struct cmd * options);

// 由已知的协议，服务器FQDN地址，端口，文件名，获server端的文件大小,
// 其中 head 指针可传NULL，对应着长度传0
ssize_t get_file_length(char * protocol, char * fqdn, char * port, char * fname,
					char * head, size_t headlen);

// 测试能否进行文件续传
bool test_continue(char * down_file, char * cfg_file, ssize_t file_length);

// 线程函数 在 thread_download.c中实现
void * thread_download(void * task);

// 打印下载进度的函数
void print_scheduler(const struct task * tsk, unsigned short thread_num);

//创建用于回收下载线程的线程
void * thread_join(void * args);

#endif
