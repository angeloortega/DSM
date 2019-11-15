
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
// node code

int main() 
{ 
    int fd = DSM_node_init(); 
    void** local_memory; 
    char buffer[BUFFER_SIZE];
    char *request[3];
    while (1) { 
        //Read from client
        if( read(fd,&buffer,(size_t)BUFFER_SIZE) == -1)
            printf("ERROR");    
        printf("%s ", buffer);        
        parseRequest(request, buffer);
        /*
        #define INIT_MESSAGE "00\r\n%d\r\n\r\n"
        #define WRITE_MESSAGE "01\r\n%d\r\n\r\n"
        #define READ_MESSAGE "02\r\n%d\r\n\r\n"
        #define CLOSE_MESSAGE "03\r\n%d\r\n\r\n"
        #define INVALIDATE_MESSAGE "04\r\n%d\r\n\r\n"
        */
        if(strcmp(request[0],"00") == 0){
            printf("Initiallizing node\n");
            int pages_per_node = atoi(request[1]);
            char message[100];
            local_memory = malloc(sizeof(void*) * pages_per_node);
            for (int i = 0; i < pages_per_node; i++) {
                local_memory[i] = malloc(sizeof(void*) * PAGE_SIZE);
            }
            DSM_node_pages(fd,pages_per_node);
            /*
            sprintf(message,"%d", pages_per_node);
            memcpy(buffer,&message[0],(size_t) strlen(message) + 1);
            send(fd,buffer,(size_t)strlen(message) + 1 , 0); 
            */
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
                        }
                        //Unsupported request
                    }
                }
            }
        }
        //free(request[0]);
        //free(request[1]);
    }
    return 0; 
} 
