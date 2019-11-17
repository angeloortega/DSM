
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
#include "DSMLib.h"
#include <pthread.h>
#include <signal.h>
// node code
int fd; 
void** local_memory; 

void closeNode(){
    printf("Socket connection closed, freeing up memory and shutting down...\n");
    fflush(stdout);  
    DSM_node_exit(fd);
    free(local_memory);
    exit(1);
}

void exitSignal(int sig)
{
    printf("\nProgram interrupted...\n");
	closeNode();
}

int main() 
{ 
    fd = DSM_node_init();  
    char buffer[BUFFER_SIZE];
    char *request[3];
    int readAmount;
    signal(SIGINT, exitSignal);
    while (1) { 
        //Read from client
        readAmount = read(fd,&buffer,(size_t)BUFFER_SIZE);
        if(readAmount == -1)
            printf("ERROR");
        if(readAmount == 0 || *buffer == '\0'){
            closeNode();
            break;    
        }    
        printf("received %d bytes from %d\n",readAmount, fd);
        fflush(stdout);                
        parseRequest(request, buffer);
        if(strcmp(request[0],"00") == 0){
            printf("Initiallizing node...\n");
            int pages_per_node = atoi(request[1]);
            char message[100];
            local_memory = malloc(sizeof(void*) * pages_per_node);
            for (int i = 0; i < pages_per_node; i++) {
                local_memory[i] = malloc(sizeof(void*) * PAGE_SIZE);
            }
            DSM_node_pages(fd,pages_per_node);
            printf("Node successfully initialized\nwaiting for requests...\n");
        }
        else{
            if(strcmp(request[0],"01") == 0){
                //write
                char *result = strstr(buffer, "\r\n\r\n");
                void *temp_page;
                result = result + 4;
                int page = atoi(request[1]);
                memcpy(local_memory[page],result,(size_t) PAGE_SIZE); 
            }
            else{
                if(strcmp(request[0],"02") == 0){
                    int page = atoi(request[1]);
                    memcpy(buffer,local_memory[page],(size_t) PAGE_SIZE);       
                    DSM_page_read_response(fd,page, buffer,atoi(request[2]));
                    free(request[2]);
                }               
                else{
                    if(strcmp(request[0],"03") == 0){
                        //CLOSE SOCKET TODO
                    // pageUsage(atoi(request[1]));
                    }
                    else{
                        if(strcmp(request[0],"05") == 0){
                        //CLOSE SOCKET TODO
                        // pageUsage(atoi(request[1]));
                        printf("received page %s!\n",request[1]);
                        fflush(stdout);
                        free(request[2]);
                        }
                        //Unsupported request
                    }
                }
            }
        }
        free(request[0]);
        free(request[1]);
    }
    return 0; 
} 
