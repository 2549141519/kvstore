#include <stdio.h>
#include <liburing.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
//gcc -o   .... -luring

#define EVENT_ACCEPT   	0
#define EVENT_READ		1
#define EVENT_WRITE		2

#define ENTRIES_LENGTH		1024
#define BUFFER_LENGTH		1024

typedef int (*msg_handler)(char *msg, int length, char *response);
static msg_handler kvs_handler;

struct conn
{
    int fd;
    int event;
};

int set_event_recv(struct io_uring *ring,int clientfd,
                        char* buffer,int len,int flag)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    struct conn accept_info = {
		.fd = clientfd,
		.event = EVENT_READ,
	};

    io_uring_prep_recv(sqe, clientfd, buffer,len, flag);
    memcpy(&sqe->user_data, &accept_info, sizeof(struct conn));

    return 0;
}

int set_event_send(struct io_uring *ring,int clientfd,
                        char* buffer,int len,int flag)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    struct conn accept_info = {
		.fd = clientfd,
		.event = EVENT_WRITE,
	};

    io_uring_prep_send(sqe, clientfd, buffer,len, flag);
    memcpy(&sqe->user_data, &accept_info, sizeof(struct conn));

    return 0;
}

int set_event_accept(struct io_uring *ring,int serverfd,
                        struct sockaddr *addr,socklen_t *addrlen, int flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    struct conn accept_info = {
		.fd = serverfd,
		.event = EVENT_ACCEPT,
	};

    io_uring_prep_accept(sqe, serverfd, (struct sockaddr*)addr, addrlen, flags);
    memcpy(&sqe->user_data, &accept_info, sizeof(struct conn));

    return 0;
}

int init_server(unsigned short port)
{
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);	
	struct sockaddr_in serveraddr;	
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));	
	serveraddr.sin_family = AF_INET;	
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);	
	serveraddr.sin_port = htons(port);	

	if (-1 == bind(serverfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr))) {		
		perror("bind");		
		return -1;	
	}	

	listen(serverfd, 10);
	
	return serverfd;
}

int proactor_start(unsigned short port, msg_handler handler)
{
    int serverfd = init_server(port);
    kvs_handler = handler;

    struct io_uring_params params;
    memset(&params, 0, sizeof(params));

    struct io_uring ring;
    io_uring_queue_init_params(ENTRIES_LENGTH, &ring, &params);

    struct sockaddr_in clientaddr;	
	socklen_t len = sizeof(clientaddr);
	set_event_accept(&ring, serverfd, (struct sockaddr*)&clientaddr, &len, 0);

    char buffer[BUFFER_LENGTH] = {0};
	char response[BUFFER_LENGTH] = {0};

    while(1)
    {
        io_uring_submit(&ring);

        struct io_uring_cqe *cqe;
		io_uring_wait_cqe(&ring, &cqe);

        struct io_uring_cqe *cqes[128];
		int nready = io_uring_peek_batch_cqe(&ring, cqes, 128);

        for(int i = 0; i<nready; i++)
        {
            struct io_uring_cqe *entries = cqes[i];
			struct conn result;
        
            memcpy(&result, &entries->user_data, sizeof(struct conn));

            if (result.event == EVENT_ACCEPT) {

				set_event_accept(&ring, serverfd, (struct sockaddr*)&clientaddr, &len, 0);
				//printf("set_event_accept\n"); //

				int connfd = entries->res;

				set_event_recv(&ring, connfd, buffer, BUFFER_LENGTH, 0);

				
			} else if (result.event == EVENT_READ) {  //

				int ret = entries->res;

				if (ret == 0) {
					close(result.fd);
				} else if (ret > 0) {
					
					//int kvs_protocol(char *msg, int length, char *response);
					ret = kvs_handler(buffer, ret, response);
					
					set_event_send(&ring, result.fd, response, ret, 0);
				}
			}  else if (result.event == EVENT_WRITE) {
				int ret = entries->res;
				//printf("set_event_send ret: %d, %s\n", ret, buffer);

				set_event_recv(&ring, result.fd, buffer, BUFFER_LENGTH, 0);
			}
        }
        io_uring_cq_advance(&ring, nready);
    }

}
/*
int kv_store(char *msg, int length, char *response)
{
    sprintf(response,msg,length);
    return length;
}

int main()
{
    proactor_start(9999,kv_store);
}
*/