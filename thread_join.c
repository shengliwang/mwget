#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define INTER_VAL 100000 //定义检测线程退出情况的时间间隔，现在是0.1s

extern unsigned short thred_num; // 具体的定义在main文件中的全局变量上
extern void * moloc;
extern void * map;
extern size_t map_len;

void swap_tid(pthread_t *x, pthread_t * y)
{
	pthread_t tmp;
	tmp = *x;
	*x = *y;
	*y = tmp;
	return;
}

void remove_tid(pthread_t * tid, unsigned short len, pthread_t val)
{
	for(int i = 0; i < len; i++)
	{
		if(tid[i] == val)
		{
			swap_tid(&tid[i], &tid[len-1]);
			return ;
		}
	}
	return;
}
void * thread_join(void * args)
{

//	printf(	"-------------------------\n"
//			"-------thread_join-------\n"
//			"----thred_num: %d--------\n"
//			"--map_len: %ld-----------\n"
//			"-------------------------\n"
//			, thred_num, map_len);

	unsigned short num = thred_num;

	pthread_t tid[num]; // 把tid赋值给一个新的数组，以便后来对tid进行改变
	for(int i = 0; i < num; i++)
		tid[i] = ((pthread_t *)args)[i];

	void * retval = NULL;
	int ret;
	while(1)
	{
		for(int i = 0; i < num; i++)
		{
			if( (ret = pthread_tryjoin_np(tid[i], &retval)) == 0 ) 
			{  //线程退出，上面函数返回0，retal 置NULL或者为非NULL的错误退出值
				//，此时应该先检测退出值为什么，
				//1， 若retval为非NULL值，则整个进程退出，并提示用户断点续传
				//2， 若retal为NULL值，把检测的tid数组移除此线程id, 并进行下一轮的检测
				if(retval != NULL)
				{
					//下载中的某个线程出错，退出进程，并提示用户重新下载，利用断点续传下载
					printf(	"download thread error! please restart this program\n"
							"to continue download\n");
					free(moloc); //释放主线程申请的资源

					munmap(map, map_len);//释放主线程申请的资源

					exit(10);
				}
				else{
					// retval为空值，线程正常退出，移除此tid，不再对他进行检测
					remove_tid(tid, num, tid[i]);
					num --; //检测的线程少一个
				}
			}
		}

		if(num == 0) //如果没有检测线程了，则退出检测
			break;

		usleep(INTER_VAL); // 每搁0.1s检测一次下载线程有没有出错
	}

	pthread_exit(NULL);
}

