/* wrap.h */
#ifndef __WRAP_H_
#define __WRAP_H_

void
perr_exit(const char *s);

int
Accept(int sockfd, struct sockaddr * addr, socklen_t *addrlen);
 
void
Bind(int sockfd, const struct sockaddr * addr, socklen_t addrlen);
 
void
Connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen);

void
Listen(int sockfd, int backlog);

int
Socket(int domain, int type, int protocol);

ssize_t
Read(int fd, void * buf, size_t count);

ssize_t
Write(int fd, const void * buf, size_t count);

void
Close(int fd);

#endif
