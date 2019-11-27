#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>

#define BUF_SIZE 1024
#define EPOLL_SIZE 50
void error_handling(char* message);

int main(int argc, char* argv[])
{
	int serv_sock;
	char message[BUF_SIZE] = {0};
	int str_len;
	struct sockaddr_in server_addr;

	struct epoll_event* ep_events;
	struct epoll_event event;
	struct epoll_event event_out;
	int epfd, event_cnt, event_fd;

	//keyboard epoll
	struct epoll_event* ep_key = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

	if(argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));

	if(serv_sock == -1)
		error_handling("socket() error");

	if(connect(serv_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
		error_handling("connect() error");

	epfd = epoll_create(EPOLL_SIZE);
	ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	event.data.fd = 0;
	epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event);

	while(1)
	{
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);

		if(event_cnt == -1)
		{
			puts("epoll_wait() err");
			break;
		}
		for(int i = 0; i < event_cnt; i++)
		{
			memset(&message, 0, BUF_SIZE);
			if(ep_events[i].data.fd == serv_sock)
			{
				read(serv_sock, message, BUF_SIZE - 1);
				printf("%s", message);//
				fflush(stdout);
			}
			if(ep_events[i].data.fd == 0)
			{
				fgets(message, BUF_SIZE, stdin);
				if(!strcmp(message,"q\n") || !strcmp(message, "Q\n"))
					break;
				write(serv_sock, message, strlen(message) + 1);
				fflush(stdin);
			}
		}
	}
	close(serv_sock);
	return 0;
}

void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
