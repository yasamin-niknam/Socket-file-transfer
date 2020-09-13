// #include <stdio.h>  /////bayad pak she
#include <string.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> 
#include <signal.h>
#include <fcntl.h> 
#include <sys/stat.h>


#define TRUE 1 
#define FALSE 0 
#define PORT 8888 
#define INVALID -1
#define MAX_SIZE 2048
#define MAX_CLIENTS 30
#define FIRST_SIGNAL "1"

int socket_fd;
struct sockaddr_in server_addr;


void reverse(char s[])
{
    int i, j;
    char c;
 
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

char ** parse(char str[])
{
	
	int i; 
	int j=0;
	char** list;
	list = malloc(sizeof(char*)*3);
	
	for(i=0;i<3; i++) 
		list[i] = malloc(sizeof(char)*100);  

	char delim[] = " ";
	char *ptr = strtok(str, delim);

	while (ptr != NULL)
	{
		list[j] = ptr;
		j++;
		ptr = strtok(NULL, delim);
	}
	return list;
}

int initial_master_socket(struct sockaddr_in* address, int heartbeat_port)
{
	int opt = TRUE; 
	int master_socket;

	if( (master_socket = socket(PF_INET , SOCK_STREAM , 0)) == 0) 
	{ 
		write(2, "socket failed\n", 15); 
		exit(EXIT_FAILURE); 
	} 

	if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
		sizeof(opt)) < 0 ) 
	{ 
		write(2, "setsockopt\n", 12); 
		exit(EXIT_FAILURE); 
	} 
	
	address->sin_family = PF_INET; 
	address->sin_addr.s_addr = INADDR_ANY; 
	address->sin_port = htons( PORT ); 

	if (bind(master_socket, (struct sockaddr *)address, sizeof(*address))<0) 
	{ 
		write(2, "bind failed\n", 13); 
		exit(EXIT_FAILURE); 
	} 

	char temp[100] = {0};
	char num_temp[10]= {0};
	itoa(PORT, num_temp);
	strcpy(temp, "Listener on port ");
	strcat(temp, num_temp);
	int len_temp = strlen(temp);
	write(1, temp, len_temp);
		
	if (listen(master_socket, 3) < 0) 
	{ 
		write(2, "Error on listening on master socket\n", 37); 
		exit(EXIT_FAILURE); 
	} 

	write(1, "Waiting for connections ...\n", 29); 

	return master_socket;
}

void initial_alarm_socket(int heartbeat_port)
{
	if((socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) <= 0)
	{
		write(2, "Socket creation error\n", 23);
		return;
	}

	int broadcastPermission = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, 
          sizeof(broadcastPermission)) < 0)
    {
        write(2, "setsockopt() failed\n", 21);
    	return;
    }


	server_addr.sin_family = PF_INET;
	server_addr.sin_addr.s_addr =  INADDR_BROADCAST; 
	server_addr.sin_port = htons(heartbeat_port);
}

void alarm_handler(int sock)
{
    int len, n;
	char *signal = FIRST_SIGNAL;
		
    n = sendto(socket_fd, (const char *)signal, strlen(signal),  
        0, (const struct sockaddr *) &server_addr, sizeof(server_addr)); 

  	alarm(1);
}

int upload_file(char* file_name,int file_size,int sd)
{
	int fd;
	int nn;
	char buffer[MAX_SIZE] = {0};
	int n = send(sd, FIRST_SIGNAL, 1, 0);
	if(n < 0)
	{
		write(2, "Error sending 1 signal\n", 24);
		return INVALID;
	}
	
	if((fd = open(file_name, O_CREAT | O_RDWR, 0666)) < 0)
	{
		write(2, "error creating the file\n", 25);
		return INVALID;
	}

	while((file_size > 0) && (nn = recv(sd, buffer, MAX_SIZE, 0)))
	{
		if(write(fd, buffer, nn) < 0)
		{
			write(2, "error writing data to the file\n", 32);
			close(fd);
			return INVALID;
		}
		file_size -= nn;
		bzero(buffer, MAX_SIZE);

	}
	write(1, "File downloaded successfully\n", 30);
	close(fd);
	return 1;
}

int download_file(char* file_name, int sd)
{
	int fd;
	struct stat file_stat;
	int file_size = INVALID;
	char data[MAX_SIZE] = {0};
	char data_size[5] = {0};

	int x = strlen(file_name);

	if((fd = open(file_name, O_RDONLY)) < 0)
	{
		write(2, "Error opening the file\n", 24);
		itoa(INVALID, data_size);
		send(sd, data_size, sizeof(data_size), 0);
		return INVALID;
	}

	if (fstat(fd, &file_stat) < 0)
	{
		write(2, "Error fstat\n", 13);
	    return INVALID;
	}
	
	
	char num[5] = {0};
	char string_to_send[200] = {0};
	itoa((int) file_stat.st_size, num);
	strcpy(string_to_send, "File Size: ");
	strcat(string_to_send, num);
	strcat(string_to_send, " bytes\n");
	int length = strlen(string_to_send);

	write(1, string_to_send, length);

	itoa((int) file_stat.st_size, data_size);
	file_size = (int) file_stat.st_size;

	int n = send(sd, data_size, sizeof(data_size), 0);
	if(n > 0)
	{
		char buffer_1[50] = {0};
		int nn = recv(sd, buffer_1, 50, 0);
		if(nn > 0)
		{
			buffer_1[nn] = '\0';
			if(atoi(buffer_1) == 1)
			{

				while((file_size > 0) && ((n = read(fd, data, sizeof(data))) >= 0))
				{
					int nnn = send(sd, data, n, 0);
					if(nnn != n)
					{
						write(2, "Error sending the file data\n", 29);
						close(fd);
						return INVALID;
					}
					file_size -= n;
					bzero(data, MAX_SIZE);
				}

				write(1, "Data sent successfully\n", 24);
				close(fd);
				return 1;

			}
			else
			{
				write(2, "signal was not 'ready'\n", 24);
				return INVALID;
			}
		}
		else
		{
			write(2, "error recieving ready\n", 23);
			return INVALID;
		}
	}
	else
	{
		write(2, "error sending 1\n", 17);
		return INVALID;
	}
}
	
void connect_to_clients(int master_socket, struct sockaddr_in* address)
{
	fd_set readfds; 
	char buffer[1024];  
	int max_sd , sd, new_socket; 
	int addrlen = sizeof(*address); 
	int client_socket[MAX_CLIENTS] ; 
	int activity; int i; int valread; 
	char *message = "Wolcome from server \r\n"; 

	for (i = 0; i < MAX_CLIENTS; i++)
		client_socket[i] = 0; 

	while(TRUE) 
	{ 
		FD_ZERO(&readfds); 	
		FD_SET(master_socket, &readfds); 
		max_sd = master_socket; 

		for ( i = 0 ; i < MAX_CLIENTS ; i++) 
		{ 
			sd = client_socket[i]; 
			if(sd > 0) 
				FD_SET(sd , &readfds); 
			if(sd > max_sd) 
				max_sd = sd; 
		} 

		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 

		if ((activity < 0) && (errno==EINTR))
			continue;

		if ((activity < 0) && (errno!=EINTR)) 
			write(2, "Select error", 12); 
		
		if (FD_ISSET(master_socket, &readfds)) 
		{ 
			if ((new_socket = accept(master_socket, (struct sockaddr *)address, (socklen_t*)&addrlen))<0) 
			{ 
				write(2, "Accept", 6); 
				exit(EXIT_FAILURE); 
			} 
			
			char new_sock[5] = {0};
			itoa(new_socket, new_sock);
			char port_str[5] = {0};
			itoa(ntohs(address->sin_port), port_str);
			char string_to_print[300] = {0};
			strcpy(string_to_print, "New connection , socket fd is ");
			strcat(string_to_print, new_sock);
			strcat(string_to_print, " , ip is : ");
			strcat(string_to_print, inet_ntoa(address->sin_addr));
			strcat(string_to_print, " , port : ");
			strcat(string_to_print, port_str);
			strcat(string_to_print, " \n");
			int len_string_to_print = strlen(string_to_print);
			write(1, string_to_print, len_string_to_print);
			write(1, "\n", 2);

			if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
				write(2, "send\n", 6); 
				
			// write(1, "Welcome message sent successfully\n", 34); 
				
			for (i = 0; i < MAX_CLIENTS; i++) 
			{ 
				if( client_socket[i] == 0 ) 
				{ 
					client_socket[i] = new_socket; 
					bzero(string_to_print, 300);
					bzero(new_sock, 5);
					itoa(i, new_sock);
					strcpy(string_to_print, "Adding to list of sockets as ");
					strcat(string_to_print, new_sock);
					strcpy(string_to_print, "\n");
					len_string_to_print = strlen(string_to_print);
					write(1, string_to_print, len_string_to_print);

					break; 
				} 
			} 
		} 
			
		for (i = 0; i < MAX_CLIENTS; i++) 
		{ 
			sd = client_socket[i]; 
				
			if (FD_ISSET( sd , &readfds)) 
			{ 
				if ((valread = recv( sd , buffer, 1024, 0)) <= 0) 
				{ 
					getpeername(sd , (struct sockaddr*)address, (socklen_t*)&addrlen); 
					
					char port_str[5] = {0};
					itoa(ntohs(address->sin_port), port_str);
					char string_to_print[300] = {0};
					strcpy(string_to_print, "Host disconnected , ip ");
					strcat(string_to_print, inet_ntoa(address->sin_addr));
					strcat(string_to_print, " , port ");
					strcat(string_to_print, inet_ntoa(address->sin_addr));
					strcat(string_to_print, " \n");

					close( sd ); 
					client_socket[i] = 0; 
				} 
				else
				{ 
					buffer[valread] = '\0'; 
					char** response = parse(buffer);
					if(strcmp(response[0], "download") == 0)
						download_file(response[1], sd);

					else if(strcmp(response[0], "upload") == 0)
						upload_file(response[1], atoi(response[2]), sd);

					else
					{
						int lk = strlen(response[0]);
					} 
				}
			} 
		}
	}
}

int main(int argc, char *argv[]) 
{ 
	int master_socket; 
	int heartbeat_port = atoi(argv[1]);


	struct sockaddr_in address; 

	initial_alarm_socket(heartbeat_port);
	alarm(1);
	signal(SIGALRM, alarm_handler);
	
	master_socket = initial_master_socket(&address, heartbeat_port);
	connect_to_clients(master_socket, &address);	

  	close(socket_fd);

	return 0; 
} 
