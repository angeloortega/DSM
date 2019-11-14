#include "Config.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h> /* Added for the nonblocking socket */
#include "DSMLib.h"
#include <unistd.h>
#include <arpa/inet.h>


void parseRequest(char* result[],char *request){
    const char * curLine = request;
    char type[3];
    int i = 0;
    if(request && request+1){
        type[0] = *request;
        type[1] = *(request+1);
        type[2] = '\0';
    }
    result[0] = type;
    result[1] = malloc(11);
    request = request + 3;
    
    while(request != '\0' && *request != '\r'){
            result[1][i] = *request; 
            request++;
            i++;
    }
    result[1][i] = '\0';
} 

void DSM_node_exit(int socket_fd){
    char *net_buf = malloc(BUFFER_SIZE);
    sprintf(net_buf,CLOSE_MESSAGE,(int) socket_fd);
    int message_length = strlen((char *)net_buf);
    write(socket_fd,&net_buf, message_length);
    close(socket_fd);
    free(net_buf);
}

int DSM_node_pages (int socket_fd, int amount){
    char *net_buf = malloc(BUFFER_SIZE);
    sprintf(net_buf,INIT_MESSAGE, amount); //Sends the node the amount of pages to reserve
    int message_length = strlen((char *)net_buf);
    write(socket_fd,&net_buf, message_length);
    read(socket_fd,&net_buf,BUFFER_SIZE); //Determines if the node was able to open the pages (net_buf > 0)
    message_length = strlen((char *)net_buf);
    int number = 0;
    for(int i=0; i < message_length; i++){
		number = number * 10 + (net_buf[i] - '0' );
	}
    return number;
}

int DSM_node_init(void){
    struct sockaddr_in addr_con; 
    int addrlen = sizeof(addr_con); 
    addr_con.sin_family = AF_INET; 
    addr_con.sin_port = htons(PORT); 
    addr_con.sin_addr.s_addr = inet_addr(IP); 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
            printf("\nfile descriptor not received!!\n"); 
        else
            printf("\nfile descriptor %d received\n", sockfd);
        if((connect(sockfd, (struct sockaddr *) &addr_con, addrlen)) == -1){
            perror("Can't connect to server: ");
            exit(1);
    }    
    //fcntl(sockfd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/
    return sockfd; //Opens the socket
}

void *DSM_page_read(int socket_fd,int page){
    void *net_buf = malloc(BUFFER_SIZE);
    sprintf((char *)net_buf,READ_MESSAGE,page);
    int message_length = strlen((char *)net_buf);
    write(socket_fd,&net_buf, message_length);
    read(socket_fd,&net_buf,BUFFER_SIZE);
    return net_buf;
}

void DSM_page_write(int socket_fd,int page, void * body){
    void *net_buf = malloc(BUFFER_SIZE);
    sprintf((char *)net_buf,WRITE_MESSAGE,page);
    int message_length = strlen((char *)net_buf);
	memcpy(&((char *)net_buf)[message_length], &((char*)body)[0], PAGE_SIZE);
    write(socket_fd,&net_buf, PAGE_SIZE + message_length);
}


void DSM_page_invalidate(int socket_fd,int page){
    void *net_buf = malloc(BUFFER_SIZE);
    sprintf((char *)net_buf,INVALIDATE_MESSAGE,page);
    int message_length = strlen((char *)net_buf);
    write(socket_fd,&net_buf, message_length);
}