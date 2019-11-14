#include "Config.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/sem.h>
#include <sys/shm.h>

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
    int sockfd; 
    struct sockaddr_in addr_con; 
    int addrlen = sizeof(addr_con); 
    addr_con.sin_family = AF_INET; 
    addr_con.sin_port = htons(PORT); 
    addr_con.sin_addr.s_addr = inet_addr(IP); 
    return socket(AF_INET, SOCK_STREAM, 0); //Opens the socket
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
	memcpy(&net_buf[message_length], &body[0], PAGE_SIZE);
    write(socket_fd,&net_buf, PAGE_SIZE + message_length);
}


void DSM_page_invalidate(int socket_fd,int page){
    void *net_buf = malloc(BUFFER_SIZE);
    sprintf((char *)net_buf,INVALIDATE_MESSAGE,page);
    int message_length = strlen((char *)net_buf);
    write(socket_fd,&net_buf, message_length);
    return net_buf;
}