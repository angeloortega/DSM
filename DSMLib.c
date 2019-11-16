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
    char *type = malloc(sizeof(char*) * 3);
    int i = 0;
    if(request && request+1){
        type[0] = *request;
        type[1] = *(request+1);
        type[2] = '\0';
    }
    result[0] = type;
    result[1] = malloc(11);
    request = request + 4;
    
    while(request != '\0' && *request != '\r'){
            result[1][i] = *request; 
            request++;
            i++;
    }
    result[1][i] = '\0';
    i = 0;
    request = request + 2;
    if(strcmp(result[0],"02") == 0 || strcmp(result[0],"05") == 0){ 
        //#define READ_MESSAGE "02\r\n%d\r\n%d\r\n\r\n" //Read local page n, send it to m
        result[2] = malloc(11);
        while(request != '\0' && *request != '\r'){
            result[2][i] = *request; 
            request++;
            i++;
        }
    }
} 

void DSM_node_exit(int socket_fd){
    char net_buf[BUFFER_SIZE];
    sprintf(net_buf,CLOSE_MESSAGE,(int) socket_fd);
    int message_length = strlen((char *)net_buf) + 1;
    send(socket_fd,net_buf, message_length,0);
    close(socket_fd);
}

void DSM_node_pages (int socket_fd, int amount){
    char net_buf[BUFFER_SIZE];
    sprintf(net_buf,INIT_MESSAGE, amount); //Sends the node the amount of pages to reserve
    int message_length = strlen(net_buf) + 1; 
    send(socket_fd,net_buf, message_length,0);
    
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

void DSM_page_read(int socket_fd,int page, int recipient){
    char net_buf[BUFFER_SIZE];
    sprintf((char *)net_buf,READ_MESSAGE,page,recipient);
    int message_length = strlen((char *)net_buf) + 1;
    send(socket_fd,&net_buf, message_length,0);
}

void DSM_page_read_response(int socket_fd,int page, void * body, int recipient){
    char net_buf[BUFFER_SIZE];
    sprintf((char *)net_buf,READ_RESPONSE,page,recipient);
    int message_length = strlen(net_buf);
	memcpy(&((char *)net_buf)[message_length], body, PAGE_SIZE);
    send(socket_fd,&net_buf, PAGE_SIZE + message_length, 0);
}

void DSM_page_write(int socket_fd,int page, void * body){
    char net_buf[BUFFER_SIZE];
    sprintf((char *)net_buf,WRITE_MESSAGE,page);
    int message_length = strlen(net_buf);
	memcpy(&((char *)net_buf)[message_length], body, PAGE_SIZE);
    send(socket_fd,&net_buf, PAGE_SIZE + message_length, 0);
}


void DSM_page_invalidate(int socket_fd,int page){
    char net_buf[BUFFER_SIZE];
    sprintf((char *)net_buf,INVALIDATE_MESSAGE,page);
    int message_length = strlen((char *)net_buf) + 1;
    send(socket_fd,&net_buf, message_length, 0);
}