#include <errno.h> 
#include <fcntl.h> 
#include <string.h>  
#include <stdlib.h> 
#include <signal.h>
#include <unistd.h>
#include <unistd.h>  
#include <sys/time.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <signal.h> 
#include <netinet/in.h> 
#include <sys/stat.h>

#define TRUE 1 
#define FALSE 0 
#define PORT 8888 
#define INVALID -1
#define PEER_MODE 1
#define MAX_SIZE 2048
#define SERVER_MODE 0
#define MAX_CLIENTS 30
#define EXAMPLE_PORT 6000
#define EXAMPLE_GROUP "239.0.0.1"

int socket_fd;
int client_port;
char* wanted_file;
int heartbeat_port;
int broadcast_port;


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
	int length = strlen(str);
	if(str[length - 1] == '\n')
		str[length - 1] = '\0';
	int i; 
	int j=0;
	char** list;
	list = malloc(sizeof(char*) * 4);
	
	for(i=0;i < 4; i++) 
		list[i] = malloc(sizeof(char) * 100);  

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

int initial_TCP_client(struct sockaddr_in* client_addr, int selected_port)
{
	int sock = 0;
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
	{
		write(2, "Socket creation error.\n", 24);
		return INVALID;
	}

	client_addr->sin_family = PF_INET;
	client_addr->sin_port = htons(selected_port);

	if(inet_pton(PF_INET, "127.0.0.1", &client_addr->sin_addr)<=0)  
	{ 
		write(2, "Invalid address/ Address not supported \n", 41); 
		return INVALID; 
	} 

	if (connect(sock, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) 
	{ 
		write(2, "Connection Failed. \n", 21); 
		return INVALID; 
	} 

	return sock;
}

int download_file_peer_part_handler(char* file_name, int sd)
{
	int fd;
	struct stat file_stat;
	int file_size = INVALID;
	char data[MAX_SIZE] = {0};
	char data_size[5] = {0};

	int x = strlen(file_name);
	if((fd = open(file_name, O_RDONLY)) < 0)
	{
		write(2, "Error opening the file.\n", 25);
		itoa(INVALID, data_size);
		send(sd, data_size, sizeof(data_size), 0);
		return INVALID;
	}

	if (fstat(fd, &file_stat) < 0)
	{
		write(2, "Error fstat.\n", 14);
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

			if(atoi(buffer_1) == 1)
			{

				while((file_size > 0) && ((n = read(fd, data, sizeof(data))) >= 0))
				{
					int nnn = send(sd, data, n, 0);
					if(nnn != n)
					{
						write(2, "Error sending the file data\n", 28);
						close(fd);
						return INVALID;
					}
					file_size -= n;
					bzero(data, MAX_SIZE);
				}

				write(1, "File sent successfully\n", 24);
				close(fd);
				return 1;

			}
			else
			{
				write(2, "Signal was not 'ready'\n", 24);
				return INVALID;
			}
		}
		else
		{
			write(2, "Error recieving ready\n", 23);
			return INVALID;
		}
	}
	else
	{
		write(2, "Error sending 1\n", 17);
		return INVALID;
	}
}

int upload_file(char* file_name, int sock)
{
	int n;
	int fd;
	int valread;
	int file_size = INVALID;
	char data_size[5] = {0};
	char buffer[10] = {0};
	char data[MAX_SIZE] = {0};
	struct stat file_stat;
	char sending_str[] = "upload ";
	strcat(sending_str, file_name);

	if((fd = open(file_name, O_RDONLY)) < 0)
	{
		write(2, "Error opening the file\n", 24);
		itoa(INVALID, data_size);
		return INVALID;
	}

	if (fstat(fd, &file_stat) < 0)
	{
	    write(2, "Error fstat\n", 13);
	    return INVALID;
	}

	char str_needed_to_be_printed[100] = {0};
	char size[5] = {0};
	itoa((int) file_stat.st_size, size);
	strcpy(str_needed_to_be_printed, "File Size: ");
	strcat(str_needed_to_be_printed, size);
	strcat(str_needed_to_be_printed, ".\n");
	int length = strlen(str_needed_to_be_printed);
	write(1, str_needed_to_be_printed, length);
	
	itoa((int) file_stat.st_size, data_size);
	strcat(sending_str, " ");
	strcat(sending_str, data_size);
	file_size = (int) file_stat.st_size;

	if (send(sock, sending_str, strlen(sending_str), 0) < 0)
	{
		write(2, "Error sending the command\n", 27);
		close(fd);
		return INVALID;
	}

	valread = recv( sock, buffer, 10, 0);
	buffer[valread] = '\0';
	if(valread < 0 || atoi(buffer) != 1)
	{
		write(2, "Error recieving the acception\n", 31);
		close(fd);
		return INVALID;	
	} 


	while((file_size > 0) && ((n = read(fd, data, sizeof(data))) >= 0))
	{
		if(send(sock, data, n, 0) != n)
		{
			write(2, "Error sending the data\n", 24);
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

int download_file(char* file_name, int sock, int mode)
{
	int n;
	int fd;
	int valread;
	int data_size;
	char buffer[10] = {0};
	char data[MAX_SIZE] = {0};
	char sending_str[] = "download ";
	char ready_str[] = "1";
	strcat(sending_str, file_name);
	if(mode == SERVER_MODE)
	{
		if (send(sock, sending_str, strlen(sending_str), 0) < 0)
		{
			write(2, "Error sending 1 signal\n", 24);
			return INVALID;
		}


	}
		valread = recv( sock, buffer, 10, 0);
		buffer[valread] = '\0';
		
		if(valread < 0 || atoi(buffer) == INVALID)
		{
			write(2, "Server does not have the file\n", 31);
			return INVALID;	
		}
		data_size = atoi(buffer);		

	if (send(sock, ready_str, strlen(ready_str), 0) < 0)
	{
		write(2, "Error sending the ready signal\n", 27);
		return INVALID;
	}
	if((fd = open(file_name, O_CREAT | O_RDWR, 0666)) < 0)
	{
		write(2, "Error creating the file\n", 25);
		return INVALID;
	}

	bzero(data, MAX_SIZE);
	while((data_size > 0) && ((n = recv(sock, data, MAX_SIZE, 0)) > 0))
	{
		if((write(fd, data, n)) < 0)
		{
			write(2, "Error writing data to the file\n", 32);
			close(fd);
			return INVALID;
		}
		data_size -= n;
		bzero(data, MAX_SIZE);
	}
	write(1, "File downloaded successfully\n", 30);
	close(fd);
	return 1;
}

void initial_alarm_socket()
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

}

void alarm_handler(int sock)
{
	int fd;
	if((fd = open(wanted_file, O_RDONLY)) > 0)
	{
		close(fd);
		alarm(0);
		return;
	}

	char* sending_str = malloc(sizeof(&wanted_file));
	strcpy(sending_str, wanted_file);
	char number[5];
	itoa(client_port, number);	
	strcat(sending_str, " ");
	strcat(sending_str, number);

	struct sockaddr_in client_address;
	int n,len;
	client_address.sin_family = PF_INET;
	client_address.sin_addr.s_addr = INADDR_BROADCAST;
	client_address.sin_port = htons(broadcast_port);

	n = sendto(socket_fd, (const char *)sending_str, strlen(sending_str),
		0, (const struct sockaddr *) &client_address, sizeof(client_address)); 

  	alarm(1);
}

void download_file_peer_mode()
{
	alarm(1);
	signal(SIGALRM, alarm_handler);	
}

void read_from_terminal(int sock)
{
	int n;
	char ** list;
	char command[MAX_SIZE] = {0};
    if((n = read(0, command, MAX_SIZE)) < 0)
    {
    	write(2, "An error occurred in reading your command.\n", 44);
    	return;
    }
    
    list = parse(command);
    int x = strlen(list[1]);

    if (strcmp(list[0], "upload") == 0)
    {
    	if(sock > 0)
	    	upload_file(list[1], sock);
	    else
	    {
	    	write(2, "Cannot uplode anything while server is down!\n", 46);
	    }
    }
    else if (strcmp(list[0], "download") == 0)
    {
    	int temp = 0;
    	if (sock > 0)
	    	temp = download_file(list[1], sock, SERVER_MODE);
	    if ((sock < 0) || (temp < 0))
	    {
	    	wanted_file = malloc(sizeof(&list[1]));
	    	strcpy(wanted_file, list[1]);
	    	int x = strlen(list[1]);
	    	wanted_file[x] = '\0';
	    	initial_alarm_socket();
	    	download_file_peer_mode();
	    }
    }
    else
    {
    	write(2, "Invalid input.\n", 16);
    	return;	
    }
}

int initial_listener(struct sockaddr_in * servaddr, int selected_port)
{
	int sockfd; 
	int opt = TRUE;
	struct timeval timeout;
	struct ip_mreq mreq;
	if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0 )
	{ 
		write(2, "socket creation failed\n", 24); 
		exit(EXIT_FAILURE); 
	}

	servaddr->sin_family = PF_INET; 
	servaddr->sin_port = htons(selected_port); 
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	int broadcast = 1;

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
		sizeof(opt)) < 0 ) 
	{ 
		write(2, "setsockopt\n", 12); 
		exit(EXIT_FAILURE); 
	}         

	if (bind(sockfd, (struct sockaddr *)servaddr, sizeof(*servaddr)) < 0)
		write(2, "bind() failed\n", 15);

	mreq.imr_multiaddr.s_addr = inet_addr(EXAMPLE_GROUP);         
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);         
	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq)) < 0) 
	{
		write(2, "setsockopt mreq\n", 17);
		exit(1);
	}

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) 
	{
		write(2,"setsockopt timeout\n", 20);
		exit(1);
	}

	return sockfd;
}

int check_if_file_exists(char* name_of_file)
{
	int fd;
	int file_size;
	struct stat file_stat;
	
	if((fd = open(name_of_file, O_RDONLY)) < 0)
	{
		return INVALID;
	}

	if (fstat(fd, &file_stat) < 0)
	{
		write(2, "Error fstat\n", 13);
	    return INVALID;
	}
	


	char string_to_print[200] = {0};
	char num[5]= {0};
	itoa((int) file_stat.st_size, num);
	strcpy(string_to_print, "This client have the file. File Size: ");
	strcat(string_to_print, num);
	strcat(string_to_print, " bytes\n");
	int length = strlen(string_to_print);
	write(1, string_to_print, length);

	file_size = (int) file_stat.st_size;
	close(fd);
	
	return file_size;
}

int check_for_requests(int broadcast_sock, struct sockaddr_in broadcast_addr)
{
	int n;
	char buffer[MAX_SIZE]= {0}; 
	int len = sizeof(broadcast_addr);
	if ((n = recvfrom(broadcast_sock, buffer, MAX_SIZE, 0,(struct sockaddr *) &broadcast_addr, (socklen_t *) &len) ) < 0)
	{
		write(2, "recvfrom() in broadcast failed\n", 32);
		return INVALID;
	}
	else
	{
		buffer[n] = '\0';
		char ** list = parse(buffer);
		char * name_of_file = malloc(sizeof(&list[0]));
		strcpy(name_of_file, list[0]);

		int peer_port = atoi(list[1]); 

		if (peer_port == client_port)
		{
			return INVALID;
		}
		else
		{
			int file_existance = check_if_file_exists(name_of_file);
			if (file_existance == INVALID)
				return INVALID;

			struct sockaddr_in peer_addr;
			int peer_sock = initial_TCP_client(&peer_addr, peer_port);
			if (send(peer_sock, name_of_file, sizeof(name_of_file), 0) < 0)
			{
				write(2, "Error sending I have\n", 22);
				return INVALID;
			}

			char buffer_1[5] = {0};
			int nn;
			if ((nn = recv(peer_sock, buffer_1, 5, 0)) < 0)
			{
				write(2, "Error recieving acception\n", 27);
				return INVALID;	
			}

			buffer_1[nn] = '\0';
			if (atoi(buffer_1) == 1)
			{
				download_file_peer_part_handler(name_of_file, peer_sock);
			}


		}
	}
}

int initial_master_socket(int client_port, struct sockaddr_in* address)
{
	int opt = TRUE; 
	int master_socket;

	if( (master_socket = socket(PF_INET , SOCK_STREAM , 0)) == 0) 
	{ 
		write(2, "socket failed\n",15); 
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
	address->sin_port = htons( client_port ); 

	if (bind(master_socket, (struct sockaddr *)address, sizeof(*address))<0) 
	{ 
		write(2, "bind failed\n", 13); 
		exit(EXIT_FAILURE); 
	} 

	char temp[100] = {0};
	char num_temp[10]= {0};
	itoa(client_port, num_temp);
	strcpy(temp, "Listener on port ");
	strcat(temp, num_temp);
	int len_temp = strlen(temp);
	write(1, temp, len_temp);
		
	if (listen(master_socket, 3) < 0) 
	{ 
		write(2, "Error on listen\n", 17); 
		exit(EXIT_FAILURE); 
	} 

	write(1, "Waiting for connections ...\n", 29); 

	return master_socket;
}

void connect_to_clients(int sock, int master_socket, struct sockaddr_in* address,
						int  broadcast_sock, struct sockaddr_in broadcast_addr)
{
	fd_set readfds; 
	char buffer[MAX_SIZE];  
	int max_sd , sd, new_socket; 
	int addrlen = sizeof(*address); 
	int client_socket[MAX_CLIENTS] ; 
	int activity; int i; int valread; 

	for (i = 0; i < MAX_CLIENTS; i++)
		client_socket[i] = INVALID; 

	while(TRUE) 
	{ 
		FD_ZERO(&readfds); 	
		FD_SET(STDIN_FILENO, &readfds); 
		FD_SET(master_socket, &readfds); 
		FD_SET(broadcast_sock, &readfds); 
		if (master_socket > broadcast_sock)
			max_sd = master_socket; 
		else
			max_sd = broadcast_sock; 

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
			write(2, "Select error\n", 14); 
		
		if (FD_ISSET(master_socket, &readfds)) 
		{ 
			if ((new_socket = accept(master_socket, (struct sockaddr *)address, (socklen_t*)&addrlen))<0) 
			{ 
				write(2, "Error in accepting new sockets\n", 32); 
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

				
			for (i = 0; i < MAX_CLIENTS; i++) 
			{ 
				if (client_socket[i] == INVALID) 
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
		
		if (FD_ISSET(STDIN_FILENO, &readfds))
			read_from_terminal(sock);

		if (FD_ISSET(broadcast_sock, &readfds))
			check_for_requests(broadcast_sock, broadcast_addr);


		for (i = 0; i < MAX_CLIENTS; i++) 
		{ 
			sd = client_socket[i]; 
				
			if (FD_ISSET(sd , &readfds)) 
			{ 
				if ((valread = recv( sd , buffer, MAX_SIZE, 0)) <= 0) 
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
						client_socket[i] = INVALID; 
				} 
				else
				{ 
						buffer[valread] = '\0'; 
						if(strcmp(buffer, wanted_file) == 0)
						{

							char* send_str= "1";

							int x = strlen(send_str);
							if (send(sd, send_str, x, 0) < 0)
							{
								write(2, "Error sending acception\n", 25);
							}	
							else
							{
								download_file(wanted_file, sd, PEER_MODE);
							}
						}
				}
			} 
		}
	}
}

int check_server(struct sockaddr_in server_addr, int heartbeat_sock)
{
	int n;
	char buffer[MAX_SIZE]= {0}; 
	int len = sizeof(server_addr);

	if ((n = recvfrom(heartbeat_sock, buffer, MAX_SIZE, 0,(struct sockaddr *) &server_addr, (socklen_t *) &len) ) < 0)
	{
		write(2, "recvfrom() in getting heartbeat failed\n", 40);
		write(2, "Server is DOWN!\n", 17);
		return INVALID;
	}
	else
	{
		buffer[n] = '\0';
		char string_to_print[200];
		strcpy(string_to_print, "Received: ");
		strcat(string_to_print, buffer);
		strcat(string_to_print, " \n");
		
		return atoi(buffer);
	}
}

int main(int argc, char *argv[])
{	
	int sock, valread;
	int mode = SERVER_MODE;
	char data[MAX_SIZE] = {0};
	client_port = atoi(argv[3]);	
	char buffer[MAX_SIZE] = {0};
	struct sockaddr_in servaddr; 
	heartbeat_port = atoi(argv[1]);
	broadcast_port = atoi(argv[2]);
	struct sockaddr_in client_addr;
	struct sockaddr_in broadcast_addr;

	int heartbeat_sock = initial_listener(&servaddr, heartbeat_port);
	int server_port = check_server(servaddr, heartbeat_sock);
	
	int broadcast_sock = initial_listener(&broadcast_addr, broadcast_port);
	
	if (server_port < 0)
	{
		sock = INVALID;
		mode = PEER_MODE;
	}
	if (mode == SERVER_MODE)
	{
		sock = initial_TCP_client(&client_addr, PORT);
		valread = recv( sock , buffer, MAX_SIZE, 0); 
	}

	struct sockaddr_in peer_addr;
	int master_socket = initial_master_socket(client_port, &peer_addr);
	connect_to_clients(sock, master_socket, &peer_addr, broadcast_sock, broadcast_addr);

	close(heartbeat_sock);
	close(broadcast_sock);


	return 0;
}	