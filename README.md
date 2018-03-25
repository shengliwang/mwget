# mwget
多线程下载 并支持 断点续传 程序(类unix下终端程序)<br>
名称：mwget<br>
版本：V0.1<br>
开发者：王胜利<br>
开发周期：2018-3-22至2018-3-25<br>
<br>
# 开发总体思路如下：
1，根据用户指定的URI来分析文件大小。<br>
2，根据用户指定的线程数来创建线程分段下载整个文件，最后合并起来。<br>
3，需要指示出下载进度，下载完成时所用的时间<br>
4，需要指定本程序应用的范围：如https不可下载<br>


# 程序编写及运行具体过程
1，包含相应的头文件<br>
2，定义宏来指定程序版本, 作者<br>
3, main函数<br>
4，分析用户指定参数（如果条件不合适，则打印出错信息并退出):<br>
	创建一个结构体cmd用来保存初始化后的参数<br>
	分析函数应找出使用的为http协议，其他协议目前不支持，应给出提示信息并退出<br>
	分析函数提取server的FQDN名称转化成整形的ipv4地址，ipv6地址目前不支持，如果为ipv6，给出信息并退出<br>
	分析函数应提取server的端口号，如果没有指定端口，则使用默认端口80<br>
	分析函数应提取获取文件的名字，如果没有名字，则默认获取index.html并给出提示信息<br>
	分析函数应提取下载文件另存为的文件名字，如果没有，则用上面的sever端名字<br>
	分析函数应提取下载所用的线程数，如果没有指定，则使用单线程下载，限定使用下载线程数不能超过定义的MAX_THREAD_NUM宏(util.c)<br>
	
5，根据分析用户指定的参数（命令行参数）获取到了server端的ip地址和端口号，以及要下载的文件的长度信息，和使用的线程数<br>
&nbsp&nbsp&nbsp&nbsp创建一个函数， 此函数应先发出HTTP 的 HEAD报文获取要获得的文件长度 (顺便测试服务器能不能正常连上，不行的话则报错退出 <br>
&nbsp&nbsp&nbsp&nbsp1》先比较状态码，不正确则报错退出 <br>
&nbsp&nbsp&nbsp&nbsp2》然后找长度，找不到，则退出并报错)<br>
	
6, 上面初始化工作结束后，下载之前应输出如下信息，此程序名称，版本号，开发者，使用线程数，server端信息，下载文件名称，存储文件名称，<br>

7，创建多个线程进行下载 （通过cfg配置文件进行线程间通讯，其中包含完成进度信息）<br>
&nbsp&nbsp&nbsp&nbsp每个线程中应有一个变量用来表示线程完成的进度（创建线程的时候传入），每个线程完成一次read和write后对这个变量进行增加<br>
&nbsp&nbsp&nbsp&nbsp在主线程中实现一个函数，用来检测这些变量的值，然后跟文件总长度做比较，给出进度条<br>

8，测试是否进行断点续传<br>
&nbsp&nbsp&nbsp&nbsp1, cfg 文件保存未完成下载的download文件的一些信息（如线程个数，已完成的长度），用于断点续传<br>
&nbsp&nbsp&nbsp&nbsp1， 创建通用的线程函数，用于重头下载和续传的两种模式，只要提交给他任务就行<br>
&nbsp&nbsp&nbsp&nbsp2, 检测是否断点续传的情况 (创建函数进行测试)<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp1》 download文件不存在，重头下载。<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp2》检测到download文件存在，cfg文件不存在，重新下载，并提示用户是否覆盖download文件，不覆盖则退出。<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp3》检测到download文件存在，cfg存在，则判断cfg文件能用否(cfg文件以yes开头能用，并且起大小是某个信息结构体的整数倍，程序中有说明)，<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp1》能用，则判断download文件大小是否正确，不正确提示覆盖，重头下载，正确则续传<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp2》不能用，报错，并重头下载，并且覆盖cfg文件。<br>						

10， 回收线程：<br>
&nbsp&nbsp&nbsp&nbsp可以根据线程退出值来判断下载线程有没有执行成功，如果不能执行成功（下载线程返回非NULL值），则整个进程退出，并提示用户重新启动本程序，断点续传。<br>

遇到的问题<br>
数据请求部分<br>
	192.168.2.102.52142 > 101.6.8.193.80: Flags [P.], cksum 0x317c (incorrect -> 0xa6a5), seq 0:128, ack 1, win 229, options [nop,nop,TS val 3963801377 ecr 3603563775], length 128: HTTP, length: 128<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbspGET /centos/7.4.1708/isos/x86_64/CentOS-7-x86_64-NetInstall-1708.iso HTTP/1.1<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbspRange: bytes=0-442499071<br>
&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbspHost: mirrors.tuna.tsin[!http]<br>
//对比程序中对rquest的构建，request一共128字节，sprintf导致部分数据没有被request数组保存，所以把request调大即可<br>
