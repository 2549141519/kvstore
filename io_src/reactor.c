#include "../include/server.h" //这里最后可以用cmake 测试用的时候写相对 绝对地址

#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<stdio.h>
#include <string.h>

ssize_t recv_line(int fd, char *buffer, size_t max_length,int flag) {
    size_t total_read = 0;
    char c;
    int n;

    while (total_read < max_length - 1) {
        // Read one character at a time
        n = recv(fd, &c, 1, flag);
        if (n > 0) {
            if (c == '\r') {
                // Peek at the next character
                n = recv(fd, &c, 1, MSG_PEEK);
                if (n > 0 && c == '\n') {
                    // Consume the '\n'
                    recv(fd, &c, 1, flag);
                }
                break;
            } else if (c == '\n') {
                break;
            } else {
                buffer[total_read++] = c;
            }
        } else if (n == 0) {
            // Connection closed
            break;
        } else {
            // Error occurred
            return -1;
        }
    }

    buffer[total_read] = '\0';
    return total_read;
}

typedef int (*msg_handler)(char *msg, int length, char *response);

static msg_handler kvs_handler;

int kvs_request(struct conn *c) {
	
	memset(c->writebuffer,0,BUFFER_LENGTH);
	c->writelength = kvs_handler(c->readbuffer, c->readlength, c->writebuffer);

	//sprintf(c->writebuffer,c->readbuffer,c->readlength);
	//c->writelength = c->readlength;
	return 0;
}

int kvs_response(struct conn *c) {

}
//这里可以把这个静态的handler丢给用户 用户写handler 就可以决定request_callback() 从而影响
//reponse_callback()


#define CONNECTION_SIZE  1024 //这里是直接定义好一个数组给携带这fd的conn用 不过最好不要直接用数组 内存开销太大 定制内存分配策略 链表 最好 
struct conn conn_pool[CONNECTION_SIZE] = {0};
int epollfd = 0; //epoll_ctl epoll_wait 都需要一个epollfd管理 我们在start函数里create 

int accept_callback(int fd);
int recv_callback(int fd);
int send_callback(int fd); //这三个函数指针 用于conn结构体里的回调函数


int set_event(int fd,int event,int flag)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = event;
	//flag 用于确定是epoll_ctl 添加还是修改 true添加 false修改
	if(flag)
		epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
	else
		epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);

	return 0;
}

//用于将recv send的fd添加到结构中 然后调用set_event 给epoll注册 
int event_register(int fd, int event)
{
    if(fd < 0) return -1;

    conn_pool[fd].fd = fd;
    conn_pool[fd].recv_callback = recv_callback;
    conn_pool[fd].send_callback = send_callback;

    memset(conn_pool[fd].readbuffer, 0, BUFFER_LENGTH);
	conn_pool[fd].readlength = 0;

	memset(conn_pool[fd].writebuffer, 0, BUFFER_LENGTH);
	conn_pool[fd].writelength = 0;

	set_event(fd, event, 1);
    return 0;
}





int init_server(unsigned short port) {

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
	servaddr.sin_port = htons(port); // 0-1023, 

	if (-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))) {
		printf("bind failed: %s\n", strerror(errno));
	}

	listen(sockfd, 10);
	//printf("listen finshed: %d\n", sockfd); // 3 0-2分别是标准输入 输出 错误

	return sockfd;

}//初始化一个listen fd 并返回其fd

int accept_callback(int fd)
{
    struct sockaddr_in  clientaddr;
	socklen_t len = sizeof(clientaddr);

	int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
	if (clientfd < 0) {
		printf("accept errno: %d --> %s\n", errno, strerror(errno));
		return -1;
	}

    event_register(clientfd, EPOLLIN);  // | EPOLLET 给epoll添加新接受的clientfd
	return 0;
}

int recv_callback(int fd)
{
	memset(conn_pool[fd].readbuffer, 0, BUFFER_LENGTH);
	int count = recv_line(fd, conn_pool[fd].readbuffer, BUFFER_LENGTH, 0);
	if (count == 0) { // disconnect
		//printf("client disconnect: %d\n", fd);
		close(fd);

		epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL); // unfinished

		return 0;
	} else if (count < 0) { // 

		printf("count: %d, errno: %d, %s\n", count, errno, strerror(errno));
		close(fd);
		epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);

		return 0;
	}

	
	conn_pool[fd].readlength = count;
	//printf("RECV: %s\n", conn_list[fd].rbuffer);

	kvs_request(&conn_pool[fd]);
	set_event(fd, EPOLLOUT, 0);//这里可以解决粘包问题 写一个状态机 把粘包问题丢给上层的kv_request协议
	//kv_request协议决定是否读完 并设置状态 这里根据状态判断 改为epoll_out还是epoll_in
	return count;
}

int send_callback(int fd)
{
	kvs_response(&conn_pool[fd]); //这里丢给上层协议 他去设置writebuffer和发送长度

	int count = 0;

	if (conn_pool[fd].writelength != 0) {
		count = send(fd, conn_pool[fd].writebuffer, conn_pool[fd].writelength, 0);
	}
	
	set_event(fd, EPOLLIN, 0);

	return count;

}

int reactor_start(unsigned short port, msg_handler handler)
{
	kvs_handler = handler;
    epollfd = epoll_create(1);//历史原因 原先epoll是数组 现在是链表 不写0写什么都行

    int serverfd = init_server(port);

    //接下来需要给listenfd 绑定对应的事件 即recv函数
    conn_pool[serverfd].fd = serverfd;
    conn_pool[serverfd].recv_callback = accept_callback;
	set_event(serverfd, EPOLLIN, 1);

	while (1) 
	{ // mainloop

		struct epoll_event events[1024] = {0};
		int nready = epoll_wait(epollfd, events, 1024, -1);

		int i = 0;
		for (i = 0;i < nready;i ++) 
		{
			int connfd = events[i].data.fd;

			if (events[i].events & EPOLLIN) {
				conn_pool[connfd].recv_callback(connfd);
			} 

			if (events[i].events & EPOLLOUT) {
				conn_pool[connfd].send_callback(connfd);
			}
		}
	}
}








