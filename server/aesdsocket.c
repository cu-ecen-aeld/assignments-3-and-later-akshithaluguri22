#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>

#define PORT ("9000")
#define TEMP_BUFFER_SIZE (3)
#define AESDSOCKET_FILEPATH ("/var/tmp/aesdsocketdata")

int server_socket_fd, client_socket_fd, aesdsocketdata_fd;
char *recieved_packets;

void signal_handler(int signum)
{
    	if (SIGINT == signum || SIGTERM == signum) {
    		syslog(LOG_DEBUG, "SIGINT or SIGTERM signal received\n");
		printf("SIGINT or SIGTERM occured\n");
    	}

	close(server_socket_fd);
	close(client_socket_fd);
	close(aesdsocketdata_fd);
	unlink(AESDSOCKET_FILEPATH);
	closelog();
	exit(0);
}

int main(int argc, char *argv[]){

	bool daemon_process=false;
	struct sockaddr_in Client_addr;
	socklen_t addr_size;
	struct addrinfo hints;
    	struct addrinfo *server_info;
    	int status;	
    	int enable = 1;	
	char temp_buffer[TEMP_BUFFER_SIZE];
    	int packet_size;
    	int file_size = 0;	
        char address_string[20];
        const char *client_ip;	
        
	//open syslog
	openlog(NULL, 0, LOG_USER);
	
	//check if deamon process
	if ((argc > 1) && (!strcmp("-d", (char*) argv[1]))){
		daemon_process=true;
		syslog(LOG_DEBUG, "aesdsocket as Daemon");
	}
	else{
		syslog(LOG_ERR, "Daemon failed");
		printf("Daemon failed");
	}

	//open the socket data file
	aesdsocketdata_fd= open(AESDSOCKET_FILEPATH,O_RDWR | O_CREAT | O_APPEND,0666);

	if( -1 == aesdsocketdata_fd ){
	
		syslog(LOG_ERR, "not able to open file %s",AESDSOCKET_FILEPATH);
		return -1;
	}
		
	//signal handlers
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	

	//hints
    	memset(&hints, 0, sizeof(hints));    
    	hints.ai_family = AF_INET;        
    	hints.ai_socktype = SOCK_STREAM;    
    	hints.ai_flags = AI_PASSIVE;        	
	
	//getaddrinfo
	status = getaddrinfo(NULL, PORT , &hints, &server_info);
	if (status != 0){
		syslog(LOG_ERR, "Client connection failed");
		closelog();
		return -1;
	}
	
	//create socket
	server_socket_fd = socket(server_info->ai_family,server_info->ai_socktype, server_info->ai_protocol );
	
    	if ( -1 == server_socket_fd ){
    		syslog(LOG_ERR, "Client connection failed");
    		closelog(); 
    	        return -1;
    	}

	// Reference : Linux System Programming Book
    	if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    	{
    		printf("Error : setsockopt\n");
    	}
    	
	//binding
	status= bind(server_socket_fd,server_info->ai_addr,server_info->ai_addrlen);
	if ( status != 0){
		syslog(LOG_ERR, "Client connection failed due to binding");
		closelog();
		return -1;
	}
	
	if(daemon_process){
		status = daemon(0, 0);
		if (status == -1){
			syslog(LOG_ERR, "Daemon process failed!!!");
		}
	}
	
	status = listen(server_socket_fd, 10);
    	if( status != 0 )
    	{
        	syslog(LOG_ERR, "Client connection failed");
        	closelog();
        	return -1;
    	}
    		
	freeaddrinfo(server_info);
	addr_size = sizeof Client_addr;
	
	while(1){
	
	        int count = 0;
        	int recv_buff_len = 0;
        	int prev_len = TEMP_BUFFER_SIZE;
        	bool packet_received = false;
		status=0;
		
		//accept
		client_socket_fd= accept(server_socket_fd, (struct sockaddr *)&Client_addr, &addr_size);
	 	if(client_socket_fd == -1 )
	 	{
            		syslog(LOG_ERR, "Client connection failed");
            		return -1;
        	}
        
		//for client_ip
               client_ip = inet_ntop(AF_INET, &Client_addr.sin_addr, address_string, sizeof(address_string));
        	syslog(LOG_DEBUG, "Connected with %s\n\r",client_ip );
        	
		memset(temp_buffer, '\0', TEMP_BUFFER_SIZE);

        	while( false == packet_received )
        	{
            		int i=0;
            		packet_size = 0;
            		            			
            		if(-1 == recv(client_socket_fd, &temp_buffer, TEMP_BUFFER_SIZE, 0)){
                		syslog(LOG_ERR, "recv failed");
            		}
      		      		
            		while(i< TEMP_BUFFER_SIZE)
            		{
		                packet_size++;
               		 if( '\n' == temp_buffer[i] )
               		 {
               		     packet_received = true;
               		     break;
               		 }
               		 i++;
            		}

            		if(count != 0)
            		{
                		char *ptr = realloc(recieved_packets, prev_len+packet_size);
                		if(ptr != NULL)
                		{
                 		 	recieved_packets = ptr;
                  		 	prev_len += packet_size;                    			
                		}
                		else
                		{
                			syslog(LOG_ERR, "realloc failed");

                		}
             			recv_buff_len = prev_len;
             			
            		}
            		else
            		{
            		        recieved_packets = (char *)malloc(packet_size);
                		if(NULL == recieved_packets)
                		{
                    			syslog(LOG_ERR, "malloc failed");
                		}
                		
          			memset(recieved_packets, '\0', packet_size);
                		recv_buff_len = packet_size;

            		}
            		
        		memcpy((count * TEMP_BUFFER_SIZE) + recieved_packets, temp_buffer, packet_size);
        		count++;
        		}

        		lseek(aesdsocketdata_fd, 0, SEEK_END);
        		
        		status= write(aesdsocketdata_fd, recieved_packets, recv_buff_len);
        		if( -1 == status)
        		{
        		    syslog(LOG_ERR, "write failed");
        		    printf("write failed\n");
        		}
        		file_size += status;
        		free(recieved_packets);
        		memset(temp_buffer, '\0', TEMP_BUFFER_SIZE);
        		lseek(aesdsocketdata_fd, 0, SEEK_SET);

        		char *recv_buffer = (char *)malloc(file_size);

        		if (NULL == recv_buffer) {
        			syslog(LOG_ERR, "not able to allocate memory");
         		   	printf("not able to allocate memory\n");
        		}

        		status = read(aesdsocketdata_fd, recv_buffer, file_size);
        		if (-1 == status ){
        		   syslog(LOG_ERR, "read failed");
        		   printf("read failed\n");
        		}

        		status= send(client_socket_fd, recv_buffer, file_size, 0);
        		if ( -1 == status) {
        		syslog(LOG_ERR, "send failed");
        		    printf("send failed\n");
        		}

        		free(recv_buffer);
        		syslog(LOG_DEBUG, "Connection closed %s\n",client_ip);

    		}
    return 0;
}
