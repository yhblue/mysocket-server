#include "socket_server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static void *_poll(void * ud) 
{
	struct socket_server *ss = ud;
	struct socket_message result;
	for (;;) 
	{
		int type = socket_server_poll(ss, &result, NULL); //--返回事件
		// DO NOT use any ctrl command (socket_server_close , etc. ) in this thread.
		switch (type) 
		{
			case SOCKET_EXIT:
				return NULL;
			case SOCKET_DATA:
				printf("message(%lu) [id=%d] size=%d\n",result.opaque,result.id, result.ud);
				free(result.data);  //如果调用了send函数，在函数内部会自动free，如果再free就会出错
				break;
			case SOCKET_CLOSE:
				printf("close(%lu) [id=%d]\n",result.opaque,result.id);
				break;
			case SOCKET_OPEN:
				printf("open(%lu) [id=%d] %s\n",result.opaque,result.id,result.data);
				break;
			case SOCKET_ERROR:
				printf("error(%lu) [id=%d]\n",result.opaque,result.id);
				break;
			case SOCKET_ACCEPT:
				printf("accept(%lu) [id=%d %s] from [%d]\n",result.opaque, result.ud, result.data, result.id);
				break;
		}
	}
}

static void test(struct socket_server *ss) 
{
	pthread_t pid;
	pthread_create(&pid, NULL, _poll, ss);    //_poll为线程要执行的函数 ss是传递的参数

	int c = socket_server_connect(ss,100,"127.0.0.1",80);
	printf("connecting %d\n",c);
	int l = socket_server_listen(ss,200,"127.0.0.1",8888,32);
	printf("listening %d\n",l);
	socket_server_start(ss,201,l); //--调用该函数，socket才真正开始工作
	int b = socket_server_bind(ss,300,1);
	printf("binding stdin %d\n",b);
	int i;
	for (i=0;i<100;i++) 
	{
	   socket_server_connect(ss, 400+i, "127.0.0.1", 8888);
	}
	sleep(5);
	socket_server_exit(ss);   //--退出socket服务器，_poll中的循环事件会退出，socket_server_poll返回SOCKET_EXIT

	pthread_join(pid, NULL); 
}

int main() 
{
	//--一些必须操作 把SIGPIPE信号忽略掉？
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);

	struct socket_server * ss = socket_server_create();  //--创建一个socket
	test(ss);                                            //--把socket句柄传递给test函数
	socket_server_release(ss);                           //--销毁socket_server

	return 0;
}
