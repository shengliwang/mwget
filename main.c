#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "wrap/wrap.h"
#include "util.h"

#define VERSION "V0.1" // 版本号
#define AUTHOR "Vicking" //作者

ssize_t length = 0; //要下载文件的大小 定义成全局变量，一会有用

unsigned short thred_num = 0; //全局变量，用于线程join
void * moloc = NULL; //全局变量，用于线程join
void * map = NULL; //全局变量，用于线程join
size_t map_len = 0;

void err_exit(char *s )
{
	perror(s);
	exit(9);
}
int
main(int argc, char * argv[])
{
	//打印欢迎信息
	printf(	"\n\033[31mWelcome use mwget(multi download program)\n"
			"Version: %s. Author: %s\n\033[0m", VERSION, AUTHOR);

	struct cmd options;
	init_args(argc, argv, &options); // 初始化参数，如果参数错误，则打印出错信息并退出
//	printf("serv_name: %s\nserv_port: %s\nfile_name: %s\nsave_name: %s\nthread_num: %d\n",
//					options.serv_name,
//					options.serv_port,
//					options.file_name,
//					options.save_name,
//					options.thread_num);
	if(strcmp(options.protocol, "http") != 0)
	{
		printf("\"%s\" protocol is not supported\n", options.protocol);
		exit(2);
	}
	
	//由FQDN 获得server的ip地址 为网络型大段存放的4个字节: host.h_addr_list
	struct hostent * host;
	host = gethostbyname(options.serv_name);
	if(NULL == host)
	{
		printf("server not found: %s\n", options.serv_name);
		exit(2);
	}
	
	//从server端获取文件长度，顺便测试服务器能不能连接上
	char head[1024] = {0};
	bzero(head, sizeof(head));
	length = get_file_length(options.protocol, options.serv_name,
					options.serv_port, options.file_name, head, sizeof(head));
//	printf("\n ---------------------main: server echo ------------\n%s", head);
//	printf("-----------------main: dig leng from sesrver echo ----------\n");
//	printf("file on server lenth: %ld\n", length);
//	printf("-----------------------------------\n\n");

	/* 测试服务器的ip地址 最后别忘打印链接信息的时候，把ip地址带上。
	char buf[20];
	bzero(buf, sizeof(buf));
	inet_ntop(AF_INET, host->h_addr_list[0], buf, sizeof(buf));
	printf("buf: %s\nlength: %d\n", buf, host->h_length);
	*/

	
	char down_file[1024] = {0};
	char cfg_file[1024] = {0};
	sprintf(down_file, "%s.mdownload", options.save_name);
	sprintf(cfg_file, ".%s.cfg", options.save_name);

	// 创建对配置文件进行mmap的地址
	char * cfg = NULL;
	struct task * tsk;
	pthread_t * tid = NULL;//用于回收线程

//	printf(	"----------------------------\n"
//			"-----------main.c-----------\n"
//			"-----sizeof(struct task)----\n"
//			"------- %ld ----------------\n", sizeof(struct task));

	if(test_continue(down_file, cfg_file, length) == false)// 检测是否进行续传
	{
		printf("\033[33mnow download file:\033[0m %s\n\n", down_file);
		// 不能续传
		// 1, 先删除down 文件和cfg文件
		if(access(down_file, F_OK) == 0)
			if(unlink(down_file) != 0)
			{
				perror(down_file);
				exit(7);
			}
		if(access(cfg_file, F_OK) == 0)
			if(unlink(cfg_file) != 0)
			{
				perror(cfg_file);
				exit(7);
			}
		// 2, 创建cfg文件，并且mmap到内存空间
		// 
		int cfg_fd;
		off_t off_set;
		if( (cfg_fd = open(cfg_file, O_RDWR | O_CREAT, 0644)) < 0)
			err_exit(cfg_file);
		if(write(cfg_fd, "yes", 3) < 0)
			err_exit(cfg_file);  //前三个字节写入yes用来标示这个配置文件可用
		ssize_t cfg_len = sizeof(struct task) * options.thread_num + 3;
		if( (off_set = lseek(cfg_fd, cfg_len-1, SEEK_SET) < 0) )
			err_exit("lseek"); 
		if(write(cfg_fd, "\0", 1) < 0) // 上面的lseek和这句write用来拓展这个文件
			err_exit(cfg_file);
//		printf("---------------------main: lseek--------------\n");
//		printf("cfg file len: %ld\n", cfg_len);
//		printf("lseek: cfg_fd: %d, offset: %ld, whence: %d\n", cfg_fd, cfg_len-1, SEEK_SET);
//		printf("lseek result: off_set: %ld\n\n", off_set);
		
		// 创建并拓展down file
		int downfd = open(down_file, O_WRONLY | O_CREAT, 0644);
		if(downfd < 0)
			err_exit("create download file");
		if( lseek(downfd, length-1, SEEK_SET) < 0)
			err_exit("lseek down file");
		Write(downfd, "\0", 1);
		Close(downfd);



		cfg = (char *)mmap(NULL, cfg_len, PROT_READ | PROT_WRITE, MAP_SHARED, cfg_fd, 0);
		map = (void *)cfg;
		map_len = cfg_len;
		// mmap的内存在主程序结束后别忘释放
		if(cfg == NULL)
			err_exit("mmap");
		close(cfg_fd);
		cfg += 3; // 把指针往后移动三位
		tsk = (struct task *)cfg; // 把char *类型强转并且把地址值赋值到struct task *类型的tsk中
		
		// 3, 把task结构体进行任务分配
		
		struct sockaddr_in servaddr; //创建服务器的ip地址和端口
    	bzero(&servaddr, sizeof(servaddr));
    	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = *(int *)host->h_addr_list[0];
    	servaddr.sin_port = htons(atoi(options.serv_port));

		ssize_t total = length;
		ssize_t step = length / options.thread_num;
		ssize_t end = 0;
		ssize_t offset = 0;

		// 对于如何分配每个线程所用的参数
		// end 成员前几个都是step的整数倍，而最后一个时total长度
		for(int i = 0; i < options.thread_num; i++)
		{
			tsk[i].addr = servaddr;
			tsk[i].total = total;
			if(i != (options.thread_num -1) )
				tsk[i].end = (i+1) * step -1;
			else
				tsk[i].end = total -1;

			tsk[i].start = i * step;
			tsk[i].offset = 0;
			strcpy(tsk[i].serv_file, options.file_name);
			strcpy(tsk[i].save_file, down_file);
			strcpy(tsk[i].server, options.serv_name);
			strcpy(tsk[i].server_port, options.serv_port);
		}
		// 创建线程进行下载
		// 2, 创建多个线程

		tid = (pthread_t *)malloc(sizeof(pthread_t) * options.thread_num);
		moloc = (void *)tid;

		thred_num = options.thread_num;
		
		for(int i = 0; i < options.thread_num; i++)
		{
			pthread_create(&tid[i], NULL, thread_download, (void *)&tsk[i]);
		}
		pthread_t tmp;
		pthread_create(&tmp, NULL, thread_join, (void *)tid); //创建用于回收下载线程的线程
		pthread_detach(tmp);

		// 下面开始进行重要的一项，打印下载速度，打印进度

		print_scheduler(tsk, options.thread_num);

		pthread_cancel(tmp); // 因为那个用于回收的线程没有退出的地方，所以cancel掉

		if(cfg != NULL) // munmap 刚才映射的cfg文件
			munmap(cfg, cfg_len);

	}else
	{
		// 可以续传
		//1, mmap  cfg 文件到内存
		printf("\033[33mnow continue download file:\033[0m %s\n\n", down_file);
		int cfg_fd;
		off_t off_set;
		if( (cfg_fd = open(cfg_file, O_RDWR)) < 0)
			err_exit("cfg_file in invalid, please delet it");

		struct stat st;
		stat(cfg_file, &st);
		options.thread_num = st.st_size / sizeof(struct task);

		cfg = (char *)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, cfg_fd, 0);
		map = (void *)cfg;
		map_len = st.st_size;

		if(cfg == NULL)
			err_exit("mmap cfg_file");
		Close(cfg_fd);
		cfg += 3; // 把指针往后移动三位
		tsk = (struct task *)cfg; // 把char *类型强转并且把地址值赋值到struct task *类型的tsk中

		
		// 创建线程进行下载
		// 2, 创建多个线程

		thred_num = options.thread_num;
		
		tid = (pthread_t *)malloc(sizeof(pthread_t) * options.thread_num);
		moloc = (void *)tid;

		for(int i = 0; i < options.thread_num; i++)
		{
			pthread_create(&tid[i], NULL, thread_download, (void *)&tsk[i]);
		}
		pthread_t tmp;
		pthread_create(&tmp, NULL, thread_join, (void *)tid);//创建用于回收下载线程的线程
		pthread_detach(tmp);

		// 下面开始进行重要的一项，打印下载速度，打印进度

		print_scheduler(tsk, options.thread_num);

		pthread_cancel(tmp); // 因为那个用于回收的线程没有退出的地方，所以cancel掉

		if(cfg != NULL) // munmap 刚才映射的cfg文件
			munmap(cfg, st.st_size);
	}

	//主线成退出之前把cfg文件删除，把文件名修改了
	// 1, 删除cfg文件
	if(access(cfg_file, F_OK) == 0)
		unlink(cfg_file);
	
	// 2,修改down文件的名字
	if( rename(down_file, options.save_name) < 0)
		perror("rename failed, please rename by hand!");

	return 0;
}
