#include "util.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "wrap/wrap.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define RECV_BUF 8192 //定义每次接收的buf长度 

void sys_exit(char * s) //非正常退出的函数，退出值为1 用于线程回收用
{
	perror(s);
	pthread_exit((void *)1);
}

//此函数找到报文的数据部分，并且把要写入文件的长度计算出来(把报头和数据分开)
char * get_data(char * s, size_t * len) //len为传入传出参数
{
	char * seg = strstr(s, "\r\n\r\n");
	if(seg == NULL)
		return s;

	int i;
	for(i = 0; i < *len; i++)
	{
		if(s[i] == '\r')
		{
			if(s[i+1] == '\n')
			{
				if(s[i+2] == '\r')
				{
					if(s[i+3] == '\n')
						break;
					else
						continue;
				}
				else
					continue;
			}
			else
				continue;
		}
		else
			continue;
	}
	//测试用，在函数返回之前把报文打印出来
//	printf("------------------thread: server echo (head)----------------\n");
//	write(STDOUT_FILENO, s, i+4);
	*len = *len - (i + 4); // len-i-4 为数据长度，i+4为报文长度
	seg += 4;
	return seg;
}

void close_exit(int connfd) //正常退出的函数，退出值为null
{
	Close(connfd);
	pthread_exit(NULL);
}

void * 
thread_download(void * arg)
{
	struct task * task = (struct task *)arg;

	// 创建套接字
	int connfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	// 链接服务器
	Connect(connfd, (struct sockaddr *)&task->addr, sizeof(task->addr));

	// 发送报文
	// 1，创建报文
	char request[8192] = { 0}; // request 的大小尽量调大一点，不然报文有可能发送不完全
	sprintf(request, 	"GET /%s HTTP/1.1\r\n"
						"Range: bytes=%ld-%ld\r\n"
						"Accept: */*\r\n"
						"User-Agent: mwget/0.1(mutithread download program on linux) "
						"developed by Vicking(xixi: www.vicking.pw)\r\n"
						"Accept-Encoding: identity\r\n"
						"Connection: close\r\n" // 只要服务器发送完数据，要求他主动断开，这样线程就能len返回0， 然后断开链接
						"Host: %s:%s\r\n\r\n"
						, task->serv_file
						, task->start + task->offset, task->end
						, task->server, task->server_port);
	
	// 2, 发送报文
//	printf("\n----------------thread request-------------\n%s", request);
	Write(connfd, request, sizeof(request));
//	printf("request sended\n--------------------------------------\n\n");

	ssize_t len;
	char buf[RECV_BUF];
	char err[128];
	// 3, 接收报文 并且写入到文件中
	int savefd = open(task->save_file, O_WRONLY | O_CREAT, 0644);
	if(savefd < 0)
	{
		bzero(err, sizeof(err));
		sprintf(err, "open: %s", task->save_file);
		sys_exit(err);//非正常退出用的函数，表示线程退出值非NULL 用于线程回收
	}
	off_t offset;
	if( (offset = lseek(savefd, task->start + task->offset, SEEK_SET) < 0) ) //把指针移到相应的位置上
		sys_exit("thread: lseek");
	
//	printf("----------------thread: lseek----------------\n");
//	printf("lseek: savefd: %d, offset: %ld, whence: %d\n", savefd, task->start+task->offset, SEEK_SET);
//	printf("lseek result: offset: %ld\n\n", offset);

	// 1》第一次接收数据舍并弃报头
	bzero(buf, sizeof(buf));
//	printf("---------------------ready to reve first data-----------------------\n");
	len = Read(connfd, buf, sizeof(buf));
//	printf("---------------------rcve first data down---------------------\n");
	if(len == 0)
		close_exit(connfd);  //正常关闭用的函数
	else{
		char * data = get_data(buf, &len);
		task->offset += Write(savefd, data, len); // 每写入部分字节，就把offset的值增加
	}

	// 2》 后来收的数据全部存到文件中去
	while(1)
	{
		bzero(buf, sizeof(buf));
		len = Read(connfd, buf, sizeof(buf));
		if(len == 0)
			close_exit(connfd);
		else
			task->offset += Write(savefd, buf, len);
	}
}
