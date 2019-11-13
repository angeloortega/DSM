
// client code for UDP socket programming 
#include "Config.h"
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/time.h>
#define IP_PROTOCOL 0 
#define IP_ADDRESS "127.0.0.1" // localhost 
#define PORT_NO 8080
#define NET_BUF_SIZE 524288
#define sendrecvflag 0 
#define CHUNK_SIZE 524092   

// function to clear buffer 
void clearBuf(char* b) 
{ 
    int i; 
    for (i = 0; i < NET_BUF_SIZE; i++) 
        b[i] = '\0'; 
} 

// function to receive file 
int recvFile(char* buf, int s) 
{ 
    int i; 
    unsigned char value; 
    for (i = 0; i < s; i++) { 
        value = buf[i]; 
        if (value == 0) 
            return 1; 
        else
            printf("%c", value); 
    } 
    return 0; 
} 
  
// driver code 
int main() 
{ 
    long total = 0;
    int sockfd, nBytes; 
    struct sockaddr_in addr_con; 
    int addrlen = sizeof(addr_con); 
    addr_con.sin_family = AF_INET; 
    addr_con.sin_port = htons(PORT_NO); 
    addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS); 
    unsigned char net_buf[NET_BUF_SIZE]; 
    char fileName[128];
    int chunkNumber = 0;
    FILE* fp; 
    
    int flag = 1;
    chunkNumber = 0;
    total = 0; 

    printf("\nPlease enter file name to receive or 'exit':\n");   
    scanf("%s", fileName);
    while (flag) { 
        sockfd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL);
        if (sockfd < 0) 
            printf("\nfile descriptor not received!!\n"); 
        else
            printf("\nfile descriptor %d received\n", sockfd); 
        if((connect(sockfd, (struct sockaddr *) &addr_con, addrlen)) == -1){
            perror("Can't connect to server: ");
            exit(1);
        }    
        // socket() 
        clearBuf(net_buf);

       
        if(strstr(fileName,"video")!= NULL){
            sprintf(net_buf, "GET /%s HTTP/1.1\r\nRange: bytes=%d-%d\r\n", fileName, chunkNumber*CHUNK_SIZE, chunkNumber*CHUNK_SIZE + CHUNK_SIZE);
        }
        else{
            sprintf(net_buf, "GET /%s HTTP/1.1", fileName);
        }
        write(sockfd, net_buf, NET_BUF_SIZE); 
        printf("\n---------Data Received---------\n"); 
        nBytes = read(sockfd, net_buf, NET_BUF_SIZE); 
        total += nBytes;
        // process 
        unsigned int size = nBytes;
        printf("Read %ld bytes!\n",total);
        printf("\n-------------------------------\n");
        close(sockfd);
        chunkNumber++;
        if (nBytes <= 200) {
            break; 
        } 
    }
    close(sockfd);
    return 0; 
} 
