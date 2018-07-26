
# 注意： 每个编译都加了-g，代表是debug版本，如果想要release版本，把每个编译的-g 去除即可
CFLAG = -std=c99
all:wrap/wrap.o main.o util.o test_continue.o thread_download.o print_scheduler.o thread_join.o
	gcc debug/wrap.o debug/main.o debug/util.o debug/test_continue.o \
	debug/thread_download.o debug/print_scheduler.o debug/thread_join.o -g -pthread -o debug/mwget

thread_join.o:thread_join.c
	gcc thread_join.c $(CFLAG) -c -g -pthread -o debug/thread_join.o 
print_scheduler.o:print_scheduler.c
	gcc print_scheduler.c  $(CFLAG) -c -g -o debug/print_scheduler.o
wrap/wrap.o: wrap/wrap.c
	gcc wrap/wrap.c $(CFLAG)  -c -g -o debug/wrap.o

main.o: main.c
	gcc main.c $(CFLAG)  -c -g -o debug/main.o

util.o: util.c
	gcc util.c $(CFLAG)  -c -g -o debug/util.o

test_continue.o: test_continue.c
	gcc test_continue.c $(CFLAG)  -c -g -o debug/test_continue.o

thread_download.o: thread_download.c
	gcc thread_download.c $(CFLAG)  -c -g -o debug/thread_download.o

clean:
	rm -f debug/*
	rm -f debug/.*.cfg
