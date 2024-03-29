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
void ID_port_recv_check();

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

char ID_check[3];
char port_num[10];
int serv_sock;

pthread_mutex_t mutx;

int main(int argc, char* argv[])
{
	int menu_num;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_return;
	if(argc != 3)
		err_msg("<ip> <port>");

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(-1 == connect(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
		err_msg("connect() err");
	
	pthread_mutex_lock(&mutx);
	strcpy(ID_check, "NG");

	printf("Enter ID: ");
	ID_port_recv_check();

	while(strcmp(ID_check, "NG") == 0)
	{
		printf("[%s] in use, Enter New ID: ", name);
		ID_port_recv_check();
	}
	
	pthread_mutex_unlock(&mutx);

	pthread_create(&snd_thread, NULL, send_msg, (void*)&serv_sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&serv_sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(serv_sock);
	return 0;
}

void ID_port_recv_check()
{	
	char buf[BUFSIZ];
	memset(&buf, 0, BUF_SIZE);
	fgets(name, NAME_SIZE, stdin);
	name[strlen(name) - 1] = 0;
	write(serv_sock, name, strlen(name)); //ID send
	read(serv_sock, buf, BUF_SIZE); //ID & port recv
	strcpy(ID_check, strtok(buf, " ")); //ID
	strcpy(port_num, strtok(NULL, " ")); //port
}

void* send_msg(void* arg)
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE + BUF_SIZE];
	while(1)
	{
		memset(&msg, 0 , BUF_SIZE);
		memset(&name_msg, 0, sizeof(name_msg));
		fgets(msg, BUF_SIZE, stdin);
		if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			close(sock);
			exit(1);
		}
		sprintf(name_msg, "%s: %s", name, msg);
		write(sock, name_msg, strlen(name_msg));

		printf("send_msg:");
		fputs(name_msg, stdout);
		puts("");
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
