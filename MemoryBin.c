#include "VirtualLibrary.h"


int fd; 
char** local_memory; 

void closeNode(){
    printf("Closing socket connection\n");
    fflush(stdout);  
    exitConnection(fd);
    free(local_memory);
    exit(1);
}

void interruption(int sig)
{
    printf("\n^C Pressed...\n");
	closeNode();
}

int main() 
{ 
    fd = beginConnection();  
    char buffer[BUFFER_SIZE];
    char *request[3];
    int readAmount;
    signal(SIGINT, interruption);
    while(1) { 
        printf("waiting for requests...\n");
        //Read from client
        readAmount = read(fd,&buffer,(size_t)BUFFER_SIZE);
        if(readAmount == -1){
            perror("ERROR READING:");
            closeNode();
        }
        if(readAmount == 0 || *buffer == '\0'){
            closeNode();
            break;    
        }                 
        parseRequest(request, buffer);
        printf("Received a #%s request from %d\n",request[0], fd);
        if(strcmp(request[0],"00") == 0){
            printf("Initiallizing node...\n");
            int pages_per_node = atoi(request[1]);
            char message[100];
            local_memory = malloc(sizeof(char*) * pages_per_node);
            for (int i = 0; i < pages_per_node; i++) {
                local_memory[i] = malloc(sizeof(char*) * PAGE_SIZE);
                if(!local_memory[i]){
                    printf("Memory could not be reserved!\n");
                    exit(1);
                }
            }
            sendMessage(fd,pages_per_node,BEGIN_CONNECTION);
            printf("Memory initialized correctly!\n");
        }
        else if(strcmp(request[0],"01") == 0){
                //write
                char *result = strstr(buffer, "\r\n\r\n");
                char *temp_page;
                result = result + 4;
                int page = atoi(request[1]);
                memcpy(local_memory[page],result,(size_t) PAGE_SIZE);
                printf("Received a write request from %d\n", fd);
        }
        else if(strcmp(request[0],"02") == 0){
                int page = atoi(request[1]);
                memcpy(buffer,local_memory[page],(size_t) PAGE_SIZE);       
                returnPage(fd,page, buffer,atoi(request[2]));
                free(request[2]);
                printf("Received a read request from %d\n", fd);
        }               
        else if(strcmp(request[0],"05") == 0){
            printf("received page %s!\n",request[1]);
            fflush(stdout);
            free(request[2]);
        }
        else{
            printf("Unsopported request received\n");
        }
        free(request[0]);
        free(request[1]);
    }
    return 0; 
} 
