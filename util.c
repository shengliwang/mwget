/* util.c */
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

#define MAX_THREAD_NUM 8 //定义最大线程数

// 最后在写详细的帮助文档
#define USAGE 	"Usage: mwget [-t thread_num(1~%d)] [-o save_file_name] URL\nFor example: mwget -t 4 -o hello.html http://www.baidu.com/index.html\n"

void usage_exit() // 打印用法并退出
{
	printf(USAGE, MAX_THREAD_NUM);
	exit(1);
}
void get_name_from_server(char * local_file, const char * server_file)
{
	//从server_file这样的字串中"***/*****/****/yyyy.xxx"提取yyyy.xxx保存到local_file 中去
	//我认为下面是非常好的手段
	char * tmp = NULL;
	while(1)
	{
		if( (tmp = strstr(server_file, "/")) != NULL )
		{
			server_file = tmp + 1;
			continue;
		}
		else
		{
			strcpy(local_file, server_file);
			break;
		}
	}
}

void init_args(int argc, char * argv[], struct cmd * options)
{
	// 如果没有命令行参数，打印用法并退出
	if(1 == argc)
		usage_exit();
	
	bzero(options, sizeof(struct cmd));
	argv ++; // argv[0] 省略，直接从argv[1] 开始遍历
	char url[4096]; // 用于分析URL
	while(*argv)
	{
		// 找到 -t 后面跟的线程数
		if(strcmp("-t", *argv) == 0)
		{
			if(argv[1] == NULL)
				usage_exit();
			options->thread_num = strtol(argv[1], NULL, 10);
			if(options->thread_num == LONG_MIN || options->thread_num == LONG_MAX)
			{
				perror("-t num");
				usage_exit();
			}
			if(options->thread_num > MAX_THREAD_NUM || options->thread_num <= 0)
				usage_exit();
			argv += 2; // 二级指针往后移动两下
			continue;
		}
		// 解析要存储文件的名字
		else if(strcmp("-o", *argv) == 0)
		{
			if(argv[1] == NULL)
				usage_exit();
			strcpy(options->save_name, argv[1]);
			argv += 2; // 二级指针往后移动两下
			continue;
		}
		// 解析server端的FQDN名称， 端口号，以及 要下载的文件的名字, 和协议
		// 排除ftp和https
		else if(strstr(*argv, "://"))
		{
			// 获取协议
			bzero(url, sizeof(url));
			strcpy(url, *argv);
			strtok(url, "://");
			strcpy(options->protocol, url);

			// 获取server FQDN名称
			bzero(url, sizeof(url));
			strcpy(url, *argv);
			int i = strlen(options->protocol)+3;
			for(int j = 0; i < strlen(url); i++)
			{
				if(url[i] != ':' && url[i] != '/')
				{
					options->serv_name[j] = url[i];
					j++;
				}
				else
					break;
			}
			if(strlen(url) == i+1)
			{
				argv ++;
				continue;
			}
			// 获取server 的端口号
			if( url[i] == ':'){
				i++;
				for(int j = 0; i < strlen(url); i++)
				{
					if(url[i] != '/')	
					{
						options->serv_port[j] = url[i];
						j ++;
					}
					else
						break;
				}
			}
			if(strlen(url) == i+1)
			{
				argv ++;
				continue;
			}
			// 获取要下载的文件在服务器的路径
			if( url[i] == '/')
			{
				i++;
				for(int j = 0; i < strlen(url);  i++)
				{
					options->file_name[j] = url[i];
					j ++;
				}
			}
			argv++;
			continue;
		}
		else
			usage_exit(); // 解析到不认识的参数，则提示错误并退出
	}
	// 给选项赋给默认值
	if(strlen(options->serv_port) == 0)
		strcpy(options->serv_port, "80"); // 默认端口 80
	if(strlen(options->file_name) == 0)
		strcpy(options->file_name, "index.html"); // 默认获取文件名为 index.html
	if(options->thread_num == 0)
		options->thread_num = 1; // 默认线程个数为 1
	if(strlen(options->save_name) == 0)
		get_name_from_server(options->save_name, options->file_name); // 如果本地保存文件名不指定，则用此文件在server端的名字 注意：server端的名字可能带有'/', 所以要处理以下
	
	// 检查错误
	if(strlen(options->serv_name) == 0)
		usage_exit(); //如果不能解析到服务器FQDN名称，则提示退出
}

// 由已知的协议，服务器FQDN地址，端口，文件名，获server端的文件大小
ssize_t get_file_length(
	char * protocol, char * fqdn, char * port, char * fname,
	char * head, size_t headlen)
{
	struct hostent * host;
	host = gethostbyname(fqdn);
	if(NULL == host)
	{
		printf("server not found: %s\n", fqdn);
		exit(2);
	}
	if(host->h_addrtype != AF_INET)
	{
		printf("%s: only ipv4 supported\n", fqdn);
		exit(2);
	}
	// 创建套接字
	int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	// 构建server的 addr
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = *(int *)host->h_addr_list[0];
	servaddr.sin_port = htons(atoi(port));

	// connect to server
	Connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// 给服务器发送一个 HEAD 类型的请求。

	//1, 先构建请求报文
	char request[1024];
	bzero(request, sizeof(request));
	sprintf(request,"HEAD /%s HTTP/1.1\r\n"
					"Host: %s:%s\r\n"
					"Connection: Close\r\n\r\n", fname, fqdn, port); //构建的报文，让链接不是Keep-Alive
																	//而是 Close状态
	
	//printf("\n ------------------func of get length: client request---------------\n");
	//printf("%s", request);
	//2, 发送报文
	Write(sockfd, request, strlen(request));
	printf("HTTP request is sent, waiting echo ......\n");

	// 接收服务器发来的回应报文
	char * buf = request; // 用于接收服务器发来信息的buf
	bzero(buf, sizeof(request));
	Read(sockfd, buf, sizeof(request));

	Close(sockfd);
	printf("HTTP echo is recived:\n\033[1m%s\033[0m", buf);
	if( (NULL != head) && strlen(buf) < headlen)
		strcpy(head, buf);
	
	// 分析从服务器发过来的内容，分析长度还有其他信息。
	// 信息存储在buf中

	// 简单说一下http回复报文的结构
	// 第一行  			协议/版本号 状态码 文字描述		如:		HTTP/1.1  200  OK
	// 第二行到多行 	消息报头类型： 内容						Content-Length: 761 \r\n
	// .......													Connection: Close \r\n
	// 上面是报文头部，头部与正文之间以\r\n 隔开				\r\n
	// 这里为正文部分       									正文
	
	// 其中  状态码在 >= 200 和  < 400 之间是 正常可用的，遇到其他的状态码应打印出错信息退出

	//获取状态码
	int i = 0;
	char status[8] = {0};
	for(; i < strlen(buf); i++) // 此循环可获取HTTP协议版本号
	{
		if(buf[i] == ' ')	
		{
			i++;
			break;
		}
	}
	for(int j = 0; i < strlen(buf); i++) // 此循环用来获取状态码
	{
		if(buf[i] != ' ')
		{
			status[j] = buf[i];
			j++;
		}
		else
			break;
	}
	int stat_num = atoi(status);

	if( ((stat_num / 100) != 2) && ((stat_num / 100) != 3) ) //判断当状态码不再200-399之间的时候出错退出
	{
		printf("error: status: %d\n", stat_num);
		exit(2);
	}
	
	// 解析文件长度
	// 1, 先获取含有 “Content-Length“的字串
	char * file_len;

	if( (file_len = strstr(buf, "content-length")) != NULL)
		;
	else if( (file_len = strstr(buf, "content-Length")) != NULL)
		;
	else if( (file_len = strstr(buf, "Content-length")) != NULL)
		;
	else if( (file_len = strstr(buf, "Content-Length")) != NULL)
		;
	else
	{
		printf("content-length not found\n");
		exit(2);
	}
	// 2, 从该字串中提取文件长度信息
	file_len = &file_len[15]; //把file_len 的指针移到之后，含数字的字符串之前
	char tmp[100] = {0}; //用于保存长度的字串
	for(int j = 0, t = 0; file_len[j] != '\r'; j++)
	{
		if(file_len[j] != ' ')
		{
			tmp[t] = file_len[j];
			t++;
		}
	}
	return strtol(tmp, NULL, 10);
}
