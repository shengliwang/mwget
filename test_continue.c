#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "wrap/wrap.h"

bool read_opt(char * prompt)
{
	char opt[10];
	printf("%s", prompt);
	fflush(stdout);
	bzero(opt, sizeof(opt));
	Read(STDIN_FILENO, opt, sizeof(opt));
	
	//对用户的答案进行判断
	//1, y\n   2, n\n   3, ****\n  4, \n
	while(1)
	{
		if(strcmp("y\n", opt) == 0)
			return true;
		else if(strcmp("n\n", opt) == 0)
			return false;
		else if(strcmp("\n", opt) == 0)
			return true;
		else
		{
			printf("invalid answer! %s", prompt);
			bzero(opt, sizeof(opt));
			Read(STDIN_FILENO, opt, sizeof(opt));
		}
	}
}

bool test_continue(char * down_file, char * cfg_file, ssize_t file_length)
{
	char prompt[512];
	struct stat st;
	char tmp[3];
	if(access(down_file, F_OK) != 0)
        return false; // down file 不存在，返回假， 代表要重新下载

    else if(access(cfg_file, F_OK) != 0) 
    {   
		// down file 存在   但cfg文件不存在, 提示是否覆盖down文件
		bzero(prompt, sizeof(prompt));
		sprintf(prompt, "%s file not exists, overwrite %s? [y]/n: ", cfg_file, down_file);
		if(read_opt(prompt) == true)
			return false;
		else
			exit(5);// 如果不覆盖，直接从这退出
    } else {
		// cfg文件存在, down file 也存在， 此时需要判断down file 的大小是否正确
		stat(down_file, &st);
		if(st.st_size != file_length)
		{
			// down file大小不正确，判断用户答案，确定覆盖，程序返回假，重新下载，选择“不”，
			// 程序直接从这个函数退出
			bzero(prompt, sizeof(prompt));
			sprintf(prompt, "%s size if not right, overwrite it? [y]/n: ", down_file);
			if(read_opt(prompt) == true)
				return false;
			else 
				exit(5);
		}
		else
		{
			// down file 大小正确，这时判断cfg文件能否可用，能用的标志时，前三个字节为"yes"
			// 能用的另一个标志是，大小是结构体的整数倍并且多三个字节
			int fd = open(cfg_file, O_RDONLY);
			if(fd < 0)
			{
				perror("open cfg_file");
				return false;
			}
			if (read(fd, tmp, 3) < 0)
			{
				perror("read cfg_file");
				return false;
			}
			close(fd);
			if(strcmp(tmp, "yes") == 0)
			{
				struct stat st;
				if( stat(cfg_file, &st)  < 0)
				{
					perror("stat cfg_file");
					return false;
				}
				if( (st.st_size % sizeof(struct task)) == 3 ) //文件大小是结构体的整数倍余3,能用
					return true;
				else
					return false;
			}
			else
				return false;
		}
    }
}
