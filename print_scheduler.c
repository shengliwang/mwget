#include <stdio.h>
#include <string.h>
#include "util.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define TIME_INTER 700000 // 定义打印进度条的间隔时间, 现在是0.7s

void set_file_str(char * print, size_t len, const char * src)  //实现能滚动的文件名
{ 

	if(strlen(src) <= len)
	{
		strcpy(print, src);
		for(int i = strlen(src); i < len; i ++)
			print[i] = ' ';
		return ;
	}

	static int i = 0; //此函数每次进来 i 值都不一样
	int index = i % (strlen(src) - len);
	for(int j = 0; j < len; j++)
	{
		print[j] = src[index];
		index ++;
	}

	i ++;
}

void set_size_str(char * str, ssize_t size)  // 大小占用8个字节
{
	if((size / 1024) == 0) //条件成立的话用B作为单位
		sprintf(str, "%7.2fB", (double)size);

	else if((size/1024/1024) == 0) // 用KB作为单位
		sprintf(str, "%7.2fKB", (double)size / 1024);

	else if((size/1024/1024/1024) == 0) // 用MB作为单位
		sprintf(str, "%7.2fMB", (double)size/1024/1024);

	else if((size/1024/1024/1024/1024) == 0) // 用GB作为单位
		sprintf(str, "%7.2fGB", (double)size/1024/1024/1024);

	else if((size/1024/1024/1024/1024/1024) == 0) // 用TB作为单位
		sprintf(str, "%7.2fTB", (double)size/1024/1024/1024/1024);
	else
		return;
}

void set_speed_str(char * str, ssize_t now)
{
	static ssize_t before = 0;
	ssize_t size = (long)((double)(now - before) / (TIME_INTER/(double)1000000));
	if((size / 1024) == 0) //条件成立的话用B作为单位
		sprintf(str, "%6.2fB/s", (double)size);

	else if((size/1024/1024) == 0) // 用KB作为单位
		sprintf(str, "%6.2fKB/s", (double)size / 1024);

	else if((size/1024/1024/1024) == 0) // 用MB作为单位
		sprintf(str, "%6.2fMB/s", (double)size/1024/1024);

	else if((size/1024/1024/1024/1024) == 0) // 用GB作为单位
		sprintf(str, "%6.2fGB/s", (double)size/1024/1024/1024);

	else if((size/1024/1024/1024/1024/1024) == 0) // 用TB作为单位
		sprintf(str, "%6.2fTB/s", (double)size/1024/1024/1024/1024);

	before = now;
	return;
}

void set_bar_str(char * str, ssize_t size, ssize_t done, ssize_t total)
{
	if(size <= 10)
		return;
	
	int bar_len = size - 6; //进度条的长度为size减去其他的东西（[] 和百分数）

	sprintf(str, "[%3ld%%", done*100/total);
	str += 5;
	int i;
	for(i = 0; i < (done * bar_len / total); i++)
		str[i] = '=';
	str[i-1] = '>';
	for(; i < bar_len; i++)
		str[i] = ' ';

	str[bar_len] = ']';

	return ;

}

void print_scheduler(const struct task * tsk, unsigned short thread_num)
{
	ssize_t total = tsk->total; //总的字节数
	ssize_t done  = 0; //已完成的字节数

	//进度条组成字符
	char file[21]; //最大20个字符表示文件
	char geted[10];  // 表示已经完成的字节数的字符表示 最大长度为9如：1000.96MB
	char speed[12]; // 表示 下载速度的字节 最大长度为12 如： 1000.96KB/s
	char bar[200]; // 表示进度条的最大长度
	
	struct winsize win;
	unsigned short width = 0; //窗口的宽度（以字符计）

	//如果窗口宽度小于60 则只打印进度条

	while(1)
	{
		done = 0;
		for(int i = 0; i < thread_num; i++) //获取已完成的字节数
		{
			done += tsk[i].offset;
		}
		//1, 实现滚动的文件名
		bzero(file, sizeof(file));
		set_file_str(file, sizeof(file) - 1, tsk->save_file); //每次循环文件名都不一样（能滚动）

		//2, 实现下载内容下载了多少
		bzero(geted, sizeof(geted));
		set_size_str(geted, done);

		//3, 下载速度获取 // 不是很精确，因为没有考虑程序运行的时间
		bzero(speed, sizeof(speed));
		set_speed_str(speed, done);
		
		//4, 进度条的获取
		ioctl(STDIN_FILENO, TIOCGWINSZ, &win); //获取窗口大小
		width = win.ws_col;
		bzero(bar, sizeof(bar));
		set_bar_str(bar, width - sizeof(speed) - sizeof(geted) - sizeof(file), done, total);

		//打印进度条
		printf("\r%s %s %s %s", file, bar, geted, speed);
		fflush(stdout);


		if(done == total) // 测试退出条件是否成立
			break;

		usleep(TIME_INTER) ;   // 代表进度条打印间隔时间: 现在是0.7s
	}
	printf("\n");
}
