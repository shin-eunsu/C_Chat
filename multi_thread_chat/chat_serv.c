#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20

void* handle_clnt(void* arg);
void send_msg(char* msg, int len);
void err_msg(char* msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int clnt_ports[MAX_CLNT];
char* clnt_addrs[MAX_CLNT];
char* clnt_names[MAX_CLNT];

char userID[NAME_SIZE];
char targetID[NAME_SIZE];
char recv_msg[BUF_SIZE];
char buf[BUF_SIZE];
struct sockaddr_in serv_addr, clnt_addr;
int clnt_addr_size;

void whisp_msg();

pthread_mutex_t mutx;
int serv_sock, clnt_sock;

int main(int argc, char* argv[])
{
	char clnt_name[NAME_SIZE];
	char clnt_port_tmp[10];
	char ID_check[3];
	pthread_t t_id;

	char tmp[10];

	if(argc != 2)
		err_msg("<port>");

	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(-1 == bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
		err_msg("bind() err");
	if(-1 == listen(serv_sock, 5))
		err_msg("listen() err");

	while(1)
	{
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr,
				&clnt_addr_size);
		
		pthread_mutex_lock(&mutx);
		strcpy(ID_check, "NG");
		memset(clnt_name, 0, NAME_SIZE);
		memset(clnt_port_tmp, 0, sizeof(clnt_port_tmp));

		while(!strcmp(ID_check, "NG"))
		{
			read(clnt_sock, clnt_name, NAME_SIZE); //ID recv
			if(clnt_cnt == 0)
			{
				strcpy(ID_check, "OK");
			}
			else
			{
				for(int i = 0; i < clnt_cnt; i++) //ID check
				{
					//printf("names[%d]:%s, name:%s\n", i, clnt_names[i], clnt_name);
					if(strcmp(clnt_names[i], clnt_name) != 0 )
					{
						strcpy(ID_check, "OK");
					}
					else
					{
						strcpy(ID_check, "NG");
						sprintf(clnt_port_tmp, "%d", clnt_addr.sin_port);		
						sprintf(recv_msg, "%s %s", ID_check, clnt_port_tmp);
						write(clnt_sock, recv_msg, strlen(recv_msg));
						break;
					}
				}
			}
		}
		sprintf(clnt_port_tmp, "%d", clnt_addr.sin_port);		
		//snd ID_check & port
		sprintf(recv_msg, "%s %s", ID_check, clnt_port_tmp);
		write(clnt_sock, recv_msg, strlen(recv_msg));
		printf("recv_msg:");
		fputs(recv_msg, stdout);
		puts("");

		clnt_names[clnt_cnt] = (char*)malloc(sizeof(char) * NAME_SIZE);
		clnt_addrs[clnt_cnt] = (char*)malloc(sizeof(clnt_addr.sin_addr));
		
		clnt_socks[clnt_cnt] = clnt_sock;
		clnt_ports[clnt_cnt] = clnt_addr.sin_port;
		strcpy(clnt_names[clnt_cnt], clnt_name);
		strcpy(clnt_addrs[clnt_cnt++], inet_ntoa(clnt_addr.sin_addr));
		
		pthread_mutex_unlock(&mutx);
		
		//user List
		printf(" - User List - \n");
		for(int i = 0; i < clnt_cnt; i++)
		{
			printf("(%d)name: %s, port: %d, IP: %s\n",
					i, clnt_names[i], clnt_ports[i], inet_ntoa(clnt_addr.sin_addr));
		}
		memset(&buf, 0, BUF_SIZE);
		strcpy(buf, "Login: ");
		strcpy(buf, clnt_name);

		//Login message
		for(int i = 0; i < clnt_cnt; i++)
		{
//			write(clnt_socks[i], buf, BUF_SIZE);
		}

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("[%s] Login\n", clnt_name);
	
//		printf("connect:[%s] %s(%d) (%d users)\n", clnt_name,
//				inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port, clnt_cnt);
	}
	close(serv_sock);
	return 0;
}

void* handle_clnt(void* arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];
	
	memset(&msg, 0, BUF_SIZE);
	while((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
		send_msg(msg, str_len);
	printf("handle_clnt/msg:");
	fputs(msg, stdout);
	puts("");
	pthread_mutex_lock(&mutx);

	for(i = 0; i < clnt_cnt; i++)
	{
		if(clnt_sock == clnt_socks[i])
		{
			if(str_len == 0)
			{
				printf("[%s] Logout.\n", clnt_names[i]);
			}
			while(i < clnt_cnt - 1)
			{
				clnt_socks[i] = clnt_socks[i + 1];
				clnt_ports[i] = clnt_ports[i + 1];
				clnt_addrs[i] = clnt_addrs[i + 1];
				clnt_names[i] = clnt_names[i + 1];
//				printf("(%d)name: %s, ports: %d\n",
//					i, clnt_names[i], clnt_ports[i]);
				i++;
			}
			break;
		}
	}
	clnt_cnt--;

	if(clnt_cnt == 0 && str_len == 0)
		printf("Last user logout.\n");
	
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}

void send_msg(char* msg, int len)
{
	int i, j, name_cnt;
	char buf[BUF_SIZE];
	char userList[BUF_SIZE];
	char snd_msg[BUFSIZ];

	pthread_mutex_lock(&mutx);
	
	strcpy(buf, msg);
	strtok(buf, " "); // ID
	strncpy(userID, buf, strlen(buf) - 1);// ID: -> ID
	strncpy(recv_msg, msg + strlen(buf) + 1, BUF_SIZE); //recv message(remove id)

	printf("snd_msg/clnt_all:");
	fputs(msg, stdout);
	puts("");
	if(!strncmp(recv_msg, "/w", 2))
	{
		whisp_msg();
	}
	else if(!strncmp(recv_msg, "/list", 5))
	{		
		strcpy(msg, " - User List - \n");
		for(int i = 0; i < clnt_cnt; i++)
		{
			sprintf(userList, "(%d)name: %s, port: %d, IP: %s\n",
					i, clnt_names[i], clnt_ports[i], inet_ntoa(clnt_addr.sin_addr));
			strcat(snd_msg, userList);
		}	
		for(int i = 0; i < clnt_cnt; i++)
		{
//			if(clnt_names[i] == userID)
				write(clnt_socks[i], snd_msg, BUF_SIZE);
		}
	}
	else
	{
		//send client all
		for(i = 0; i < clnt_cnt; i++)
		{
//			if(clnt_names[i] != userID)
				write(clnt_socks[i], msg, BUF_SIZE);
				printf("snd_msg/clnt_all:");
				fputs(msg, stdout);
				puts("");
		}
	}
	pthread_mutex_unlock(&mutx);
}

void whisp_msg()
{
	int i, my_cnt, w_cnt;
	char parse[BUF_SIZE];
	char snd_msg[BUF_SIZE];
	char send_whisp[BUF_SIZE];

	strncpy(parse, recv_msg + 3, BUF_SIZE);// remove /w
	strcpy(targetID, strtok(parse, " "));
	strncpy(snd_msg, recv_msg + 3 + strlen(targetID) + 1, BUF_SIZE);

	for(i = 0; i < clnt_cnt; i++)
	{
		if(clnt_sock == clnt_socks[i])
			my_cnt = i; //user clnt_sock
		if(!strcmp(clnt_names[i], targetID))
			w_cnt = i; //whisper target clnt_sock
	}

	sprintf(send_whisp, "%s 님의 귓속말: %s", clnt_names[my_cnt], snd_msg);
	write(clnt_socks[w_cnt], send_whisp, BUF_SIZE);
	fflush(stdin);
}

void err_msg(char* msg)
{	
	fputs(msg, stderr);
	puts("");
	exit(1);
}
