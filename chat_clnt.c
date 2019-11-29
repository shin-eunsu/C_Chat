#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

#define BUF_SIZE 1024
#define NAME_SIZE 20

void* send_msg(void* arg);
void* recv_msg(void* arg);
void err_msg(char* msg);
int port_num;

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
pthread_mutex_t mutx;

int main(int argc, char* argv[])
{
	int serv_sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_return;
	if(argc != 4)
		err_msg("<ip> <port> <name>");

	strcpy(name, argv[3]);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(-1 == connect(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
		err_msg("connect() err");
	
	pthread_mutex_lock(&mutx);
	write(serv_sock, argv[3], NAME_SIZE);
	pthread_mutex_unlock(&mutx);

	pthread_create(&snd_thread, NULL, send_msg, (void*)&serv_sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&serv_sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(serv_sock);
	return 0;
}

void* send_msg(void* arg)
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE + BUF_SIZE];
	while(1)
	{
		memset(&name_msg, 0, sizeof(name_msg));
		fgets(msg, BUF_SIZE, stdin);
		if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			close(sock);
			exit(1);
		}
		sprintf(name_msg, "%s: %s", name, msg);
		write(sock, name_msg, strlen(name_msg));
	}
	return NULL;
}

void* recv_msg(void* arg)
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE + BUF_SIZE];
	int str_len;
	while(1)
	{
		memset(&name_msg, 0, sizeof(name_msg));
		str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
		if(str_len == -1)
			return (void*)-1;
		name_msg[str_len] = 0;
		fputs(name_msg, stdout);
	}
	return NULL;
}

void err_msg(char* msg)
{
	fputs(msg, stderr);
	puts("");
	exit(1);
}
