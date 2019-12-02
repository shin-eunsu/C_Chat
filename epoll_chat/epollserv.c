#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>

#define BUF_SIZE 1024
#define CLNT_FD_SIZE 30
#define EPOLL_SIZE 50
void err_msg(char* msg);

int main(int argc, char* argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;
	int str_len, i;
	char buf[BUF_SIZE];
	char buf_snd[BUFSIZ];

	struct epoll_event* ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	int clnt_fd[CLNT_FD_SIZE];
	int clnt_cnt = 0;

	if(argc != 2)
		err_msg("<port");

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		err_msg("bind() err");
	if(listen(serv_sock, 5) == -1)
		err_msg("listen() err");

	epfd = epoll_create(EPOLL_SIZE);
	ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	while(1)
	{
		memset(&buf, '\0', BUF_SIZE);
		memset(&buf_snd, '\0', BUF_SIZE);
		
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt == -1)
		{
			puts("epoll_wait() err");
			break;
		}

		for(i = 0; i < event_cnt; i++)
		{
			if(ep_events[i].data.fd == serv_sock) //clnt connect check
			{
				clnt_addr_size = sizeof(clnt_addr);
				clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, 
						&clnt_addr_size);
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				clnt_fd[clnt_cnt++] = clnt_sock;
			}
			else
			{
				str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
			
				if(str_len == 0) //close request
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
					close(ep_events[i].data.fd);
				}
				else
				{
					strcat(buf_snd, inet_ntoa(clnt_addr.sin_addr));
					strcat(buf_snd, " > ");
					strcat(buf_snd, buf);
					
					fputs(buf_snd, stdout);
					
					for(int j = 0; j < clnt_cnt; j++)
					{
						if(clnt_fd[j] != ep_events[i].data.fd)
							write(clnt_fd[j], buf_snd, strlen(buf_snd) + 1);
					}
//						printf("[%s] %d\n", __func__, __LINE__);
				}
			}
		}
	}
	close(serv_sock);
	close(epfd);
	return 0;
}

void err_msg(char* msg)
{
	fputs(msg, stderr);
	puts("");
	exit(1);
}

