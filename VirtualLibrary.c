#include "Declarations.h"


void parseRequest(char* result[],char *request){
    //Gets a request from a child and populates the result with information such as message type, page and optionally a recipient (for reading a page)
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
    
    while(*request != '\0' && *request != '\r'){
            result[1][i] = *request; 
            request++;
            i++;
    }
    result[1][i] = '\0';
    i = 0;
    request = request + 2;
    if(strcmp(result[0],"02") == 0 || strcmp(result[0],"05") == 0){ 
        result[2] = malloc(11);
        while(*request != '\0' && *request != '\r'){
            result[2][i] = *request; 
            request++;
            i++;
        }
    }
} 

int beginConnection(void){
    //Create a connection and return the file descriptor if done correctly
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
    return sockfd; //Opens the socket
}

void exitConnection(int socket_fd){
    //Close a connection
    char message[BUFFER_SIZE];
    sprintf(message,CLOSE_CONNECTION,(int) socket_fd);
    int message_length = strlen((char *)message) + 1;
    send(socket_fd,message, message_length,0);
    close(socket_fd);
}

void readPage(int socket_fd,int page, int recipient){
    //Read virtual page and send it to back to recipient
    char message[BUFFER_SIZE];
    sprintf((char *)message,READ_PAGE,page,recipient);
    int message_length = strlen((char *)message) + 1;
    send(socket_fd,&message, message_length,0);
}

void returnPage(int socket_fd,int page, void * body, int recipient){
    //Node answer of virtual page, sent to recipient
    char message[BUFFER_SIZE];
    sprintf((char *)message,READ_RESULT,page,recipient);
    int message_length = strlen(message);
	memcpy(&((char *)message)[message_length], body, PAGE_SIZE);
    send(socket_fd,&message, PAGE_SIZE + message_length, 0);
}

void writePage(int socket_fd,int page, void * body){
    //When a page is modified it is stored in virtual page.
    char message[BUFFER_SIZE];
    sprintf((char *)message,WRITE_PAGE,page);
    int message_length = strlen(message);
	memcpy(&((char *)message)[message_length], body, PAGE_SIZE);
    send(socket_fd,&message, PAGE_SIZE + message_length, 0);
}

void sendMessage(int socket_fd,int page, char* MESSAGE){
    //Generic messages such as write response or reserve pages
    char message[BUFFER_SIZE];
    sprintf((char *)message,MESSAGE,page);
    int message_length = strlen(message) + 1;
    send(socket_fd,&message, message_length, 0);
}

void readResponse(ProgramInformation info){
    //Handles everything a client needs to connect to the server and read back responses
    char buffer[BUFFER_SIZE];
    char *request[3];
    int readAmount;
    //Read from server
    readAmount = recv(info->fd,&buffer,(size_t)BUFFER_SIZE,0);
    if(readAmount == -1)
        printf("ERROR");
    if(readAmount == 0 || *buffer == '\0'){
        exit(1);    
    }    
    parseRequest(request, buffer);

    if(strcmp(request[0],"05") == 0){
        printf("Received page %s!\n",request[1]);
        fflush(stdout);
        char *result = strstr(buffer, "\r\n\r\n");
        int page = atoi(request[1]);
        result = result + 4;
        memcpy(info->localMemory[page%info->pagesPerNode],result,(size_t) PAGE_SIZE);
        free(request[2]);
    }
    //Unsupported request
    free(request[0]);
    free(request[1]);
}

char* accessMemory(ProgramInformation info, int address, int writeFlag){
    //Access memory from a program. The info input contains all information regarding the program
    //Address contains the virtual address that's going to be accessed
    //writeFlag tells us if the access is to write to the page, which invalidates it.
    int page = address / PAGE_SIZE;
    if(writeFlag){
        info->pageValid[page] = 0;
    }
    int offset = address % PAGE_SIZE;
    int virtualPage = page % info->pagesPerNode;
    if(info->pageBuffer[virtualPage] != page){
        //Requested page is not in main memory, based on locality principle
        printf("Swapping page %d for page %d!\n",info->pageBuffer[virtualPage],page);
        fflush(stdout);
        if(!info->pageValid[page]){
            //Page is written to node
            writePage(info->fd, info->pageBuffer[virtualPage],info->localMemory[virtualPage]);
            readResponse(info);
            fflush(stdout);
            info->pageValid[page] = 1;
        }
        readPage(info->fd,page,info->fd);
        readResponse(info);
        fflush(stdout);
        info->pageBuffer[virtualPage] = page;
        fflush(stdout);
    }
    return &info->localMemory[virtualPage][offset];
}
int allocate(ProgramInformation info,int bytes){
    //searches for a block of contiguous memory big enough for the variable in question
    for(int i = 0; i < info->pageAmount; i++){
        Item busy = info->pageInfo[i].busyMemory;
        //busyMemory not initialized
        if(!busy && bytes < PAGE_SIZE){
            Item new = (Item) malloc(sizeof(Item));
            new->beginning = 0;
            new->end = bytes;
            new->next = NULL;
            info->pageInfo[i].busyMemory = new;
            return new->beginning + i * PAGE_SIZE;
        }
        //Check for space in beginning
        if(busy && busy->beginning > bytes){
            Item new = (Item) malloc(sizeof(Item));
            new->beginning = 0;
            new->end = bytes;
            new->next = busy;
            info->pageInfo[i].busyMemory = new;
            return new->beginning + i * PAGE_SIZE;
        }
        while(busy){
            if(busy->next){
                //Check space between nodes
                if(busy->next->beginning - busy->end  > bytes){
                    Item new = (Item) malloc(sizeof(Item));
                    new->beginning = busy->end;
                    new->end = new->beginning + bytes;
                    new->next = busy->next;
                    busy->next = new;
                    return new->beginning + i*PAGE_SIZE;
                }
            }
            else{
                //Last/only node check if enough space between node end and page end
                if(PAGE_SIZE - busy->end > bytes){
                    Item new = (Item) malloc(sizeof(Item));
                    new->beginning = busy->end;
                    new->end = new->beginning + bytes;
                    new->next = NULL;
                    busy->next = new;
                    return new->beginning + i*PAGE_SIZE;
                }
            }
            busy = busy->next;
        }

    }
    return -1; //No memory available
}

void deallocate(ProgramInformation info,int address){
    //Frees the block which contains address
    int page = (int) address / PAGE_SIZE;
    int offset = (int) address % PAGE_SIZE;
    if(address < 0){
        printf("Can't free out of bounds memory!\n");
        fflush(stdout);
        exit(1);
    }
    Item previous = info->pageInfo[page].busyMemory;
    Item busy = previous->next;
    //only 1 node
    if(!busy && previous){
        info->pageInfo[page].busyMemory = NULL;
        free(previous);
        return;
    }
    //first node
    if(previous && previous->beginning <= offset && previous->end > offset){
        info->pageInfo[page].busyMemory = busy;
        free(previous);
        return;
    }
    while(busy){
        if(busy->next){
            //Check space between nodes
            if(busy->beginning <= offset && busy->end > offset){
                previous->next = busy->next;
                free(busy);
                return;
            }
        }
        else{
            //Last node
            if(busy->beginning <= offset && busy->end > offset){
                previous->next = NULL;
                free(busy);
                return;
            }
        }
        previous = busy;
        busy = busy->next;
    }

    //busyMemory not initialized
    printf("Trying to free unallocated memory\n");
    exit(1);
}

ProgramInformation setupProgram(){
    //Initializes a program
    ProgramInformation info = malloc(sizeof(struct ProgramInformation));
    
    int totalMemory;
    int nodeAmount;
    printf("Enter the memory size in bytes: ");
    fflush(stdout);
    scanf("%d",&totalMemory);
    printf("Enter amount of nodes created: ");
    fflush(stdout);
    scanf("%d",&nodeAmount);
    info->nodeAmount = nodeAmount;

    info->pageAmount =(int) totalMemory / PAGE_SIZE;
    info->pagesPerNode = (int) info->pageAmount/info->nodeAmount;

    //Initializations
    info->pageInfo = malloc(sizeof(Page)*info->pageAmount);
    info->pageValid = (int *)malloc(info->pageAmount * sizeof(int*));
    for(int i = 0; i < info->pageAmount; i++) {
        info->pageInfo[i].busyMemory = NULL;
        info->pageValid[i] = 1;
    }

    info->localMemory = (char **)malloc(info->pagesPerNode * sizeof(char*));
    for(int i = 0; i < info->pagesPerNode; i++) info->localMemory[i] = (char *)malloc(PAGE_SIZE * sizeof(char));
    info->pageBuffer = (int *)malloc(info->pagesPerNode * sizeof(int*));

    //Memory initialization
    for(int i = 0; i < info->pagesPerNode; i++){
        info->pageBuffer[i] = i;
    }

    int DSMNode = beginConnection();
    info->fd = DSMNode;
    return info;
}


void closeProgram(ProgramInformation info){
    //Closes a program and frees memory
    exitConnection(info->fd);
    printf("Freeing up local memory and shutting down\n");
    for(int i = 0; i < info->pagesPerNode; i++) free(info->localMemory[i]);
    free(info->pageInfo);
    free(info->pageValid);
    free(info->localMemory);
    free(info->pageBuffer);
}
