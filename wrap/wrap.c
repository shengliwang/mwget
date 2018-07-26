#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "wrap.h"

void
perr_exit(const char *s)
{
	perror(s);
	pthread_exit((void *)2);
}

int
Accept(int sockfd, struct sockaddr * addr, socklen_t *addrlen)
{
	int n;

again:
	if((n = accept(sockfd, addr, addrlen)) < 0)
	{
		if((errno == ECONNABORTED) || (errno == EINTR))
			goto again;
		else
			perr_exit("accept error");
	}
	return n;
}

void
Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if(bind(sockfd, addr, addrlen) < 0)
		perr_exit("bind error");
}

void
Connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen)
{
	if(connect(sockfd, addr, addrlen) < 0)
		perr_exit("connect error");
}

void
Listen(int sockfd, int backlog)
{
	if(listen(sockfd, backlog) < 0)
		perr_exit("listen error");
}

int
Socket(int domain, int type, int protocol)
{
	int n;
	if((n = socket(domain, type, protocol)) < 0)
		perr_exit("create socket error");
	
	return n;
}

ssize_t
Read(int fd, void * buf, size_t count)
{
	ssize_t n;

again:
	if((n = read(fd, buf, count)) < 0)
	{
		if(errno == EINTR)
			goto again;
		else
			perr_exit("read error");
	}

	return n;
}

ssize_t
Write(int fd, const void * buf, size_t count)
{
	ssize_t n;

again:
	if((n = write(fd, buf, count)) < 0)
	{
		if(errno == EINTR)
			goto again;
		else
			perr_exit("write error");
	}
	return n;
}

void
Close(int fd)
{
	if(close(fd) < 0)
		perr_exit("close error");
}
