
#include "VirtualLibrary.h"
#define pagesPerNode(node,amount,pageAmount)(node >= pageAmount % amount ? (int) pageAmount/amount : (int) pageAmount/amount + 1)

static int sockfd,nodeAmount,pageAmount,memoryAmount, nodePages;
static char *logFile;
int* pageTable;
int* openSockets;
int connected;
pthread_t *threads;
sem_t semaphore;

void printLog(char* type, char* message){
	int flag;
	sem_wait(&semaphore); //Can't write to the same file at the sime time
	time_t t = time(NULL);
 	struct tm *startTime = localtime(&t);
	FILE* f = fopen(logFile,"a");
	if(f == NULL){
		f = fopen(logFile,"w");
	}
    if(f == NULL){
			perror("Logging error: ");
	}
	else{
		printf("%02d:%02d:%02d - %s: %s",  (*startTime).tm_hour,  (*startTime).tm_min,  (*startTime).tm_sec, type, message);
		fflush(stdout);
        fprintf(f, "%02d:%02d:%02d - %s: %s",  (*startTime).tm_hour, (*startTime).tm_min,  (*startTime).tm_sec, type, message);
		fclose(f);

	}
	sem_post(&semaphore);
}
void finishServer(){
    //Close server and clean up sockets and memory
    if(getpid() == getpid()){  // Only running on first thread
        for (int i=0;i<connected;i++){
            pthread_cancel(threads[i]); //Cancel any pending execution
            close(openSockets[i]);
        }
        close(sockfd);
        printLog("STATUS","server shutting down\n");
        free(openSockets);
        free(pageTable);
        sem_destroy(&semaphore);
    }
}

void exitSignal(int sig){
    printLog("STATUS","Program interrupted...\n");
	finishServer();
}

int findNode(int fd){
    for(int i = 0; i<connected; i++){
        if(openSockets[i] == fd){
            return i;
        }
    }
    return -1;
}

void initializeNodes(){
    for(int i = 0; i < nodeAmount; i++){
        sendMessage(openSockets[i], pagesPerNode(i, nodeAmount, pageAmount),BEGIN_CONNECTION);
    } 
}

void *threadHandler(void *arg){
    int n;
    char buffer[BUFFER_SIZE];
    char *request[3];
    int fd = *((int*)arg);
    int readAmount = 0;
    char message[128];
    sprintf(message,"new connection: node %d\n", fd);
    printLog("THREAD", message);
    while(1){
        //Read from client
        readAmount = read(fd,&buffer,(size_t)BUFFER_SIZE);
	//printLog("Custom", buffer);
        if(readAmount == -1){
             if(errno==EINTR || errno==9){
                printf("Interrupted thread\n");
                fflush(stdout);
                pthread_exit((void*) 1);
                break;
            }
            printLog("ERROR",strerror(errno));
            continue;    
        }
        if(readAmount == 0 || *buffer == '\0'){
            close(fd);
            printLog("ERROR",strerror(errno));
            connected--;
            break;    
        }    
        parseRequest(request, buffer);
        sprintf(message,"Received a #%s request from %d\n",request[0], fd);
        printLog("THREAD",message);
        if(strcmp(request[0],"00") == 0){
            //Node successfully created
            sprintf(message,"Node %d was able to reserve %s pages of %d bytes\n",fd, request[1], PAGE_SIZE);
            printLog("THREAD",message);
            fflush(stdout); 
        }
        else if(strcmp(request[0],"01") == 0){
            //write
            char *result = strstr(buffer, "\r\n\r\n");
            void *temp_page = malloc(sizeof(char*) * PAGE_SIZE);
            int page = atoi(request[1]);

            if(page >= pageAmount){
                printLog("ERROR","Attempting to reach out of bounds memory\n");       
                free(temp_page);
                continue;
            }
            int node =  openSockets[page%nodeAmount];
            if(node == -1){
                printLog("ERROR","Attempting to reach closed node\n");
                free(temp_page);
                continue;
            }
            result = result + 4;
            memcpy(temp_page,result,(size_t) PAGE_SIZE);
            writePage(node, (int) page/nodeAmount, temp_page);
            sendMessage(fd, (int) page, WRITE_RESULT);
            free(temp_page);
            pageTable[page] = 1;
            sprintf(message,"Write Request for page #%d\n",page);
            printLog("THREAD",message);
        }
        else if(strcmp(request[0],"02") == 0){
            int page = atoi(request[1]);

            if(page >= pageAmount){
                printLog("ERROR","Attempting to reach out of bounds memory\n");                               
                continue;
            }
            int node =  openSockets[page%nodeAmount];
            if(node == -1){
                printLog("ERROR","Attempting to reach closed node\n");
                continue;
            }
            int requester = findNode(fd);
            readPage(node,(int) page/nodeAmount, requester);
            free(request[2]);
            sprintf(message,"Read Request for page #%d\n",page);
            printLog("THREAD",message);
        }

        else if(strcmp(request[0],"03") == 0){                                      
            int node = findNode(fd);
            if(node < nodeAmount){
                sprintf(message,"Node %d closing down!\n",fd);
                printLog("ERROR",message); 
                for(int i = 0; i < pageAmount; i++){
                    pageTable[nodeAmount*i + node] = 0; //InpageTableate all pages related to node
                }
            }
            else{
                sprintf(message,"Client %d finishing up!\n",fd);
                printLog("PROGRAM",message); 
            }
            openSockets[node] = -1;
            connected--;
            close(fd);
            free(request[0]);
            free(request[1]);
            break;
        }
        else if(strcmp(request[0],"04") == 0){
            int page = atoi(request[1]);

            if(page >= pageAmount){
                printLog("ERROR","Attempting to reach out of bounds memory\n");                               
                continue;
            }
            pageTable[page] = 0;
            sprintf(message,"Invalidate Request for page #%d\n",page);
            printLog("THREAD",message);
        }
        else if(strcmp(request[0],"05") == 0){
            char *result = strstr(buffer, "\r\n\r\n");
            void *temp_page = malloc(sizeof(char*) * PAGE_SIZE);
            result = result + 4;
            int page = atoi(request[1]);

            if(page >= pageAmount){
                printLog("ERROR","Attempting to reach out of bounds memory\n");                               
                free(temp_page);
                free(request[2]);
                continue;
            }

            int source = findNode(fd);
            int destination = openSockets[atoi(request[2])];
            if(destination == -1){
                printLog("ERROR","Attempting to reach] closed node\n");                               
                free(temp_page);
                free(request[2]);
                continue;
            }
            memcpy(temp_page,result,(size_t) PAGE_SIZE); 
            returnPage(destination,(int) page*nodeAmount + source, temp_page,destination);
            free(temp_page);
            free(request[2]);
            sprintf(message,"Sending page #%d from node #%d to node #%d\n",page,source,destination);
            printLog("THREAD",message);
        }
        else{
            sprintf(message,"Received an invalid request!\n");
            printLog("THREAD",message);
        }   
        free(request[0]);
        free(request[1]);
    }
	pthread_exit(0);
}

int main(int argc, char* argv[]){
    int newFd;
    sem_init(&semaphore, 0, 1);
    nodeAmount = 0;
    memoryAmount = 0;
    for(int i=0; i<argc; ++i){   
        if(strcmp(argv[i], "-L") == 0){
            if(i + 1 < argc){
                logFile = argv[i+1];
            }
        }
        if(strcmp(argv[i], "-N") == 0){
            if(i + 1 < argc)
                nodeAmount = atoi(argv[i+1]);
        }
        if(strcmp(argv[i], "-M") == 0){
            if(i + 1 < argc)
                memoryAmount = atoi(argv[i+1]);
        }
    }
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    int sin_size;
    char buffer[BUFFER_SIZE];
    int	i;
    connected = 0;
    
	srand(time(NULL));
    signal(SIGINT, exitSignal);
    //Virtual address table creation
    pageAmount = (memoryAmount/PAGE_SIZE) + ( memoryAmount % PAGE_SIZE == 0 ? 0 : 1);
    pageTable = malloc(sizeof(int) * pageAmount);
    openSockets = malloc(sizeof(int) * nodeAmount + 1);
    threads = malloc(sizeof(pthread_t) * nodeAmount);
    char message[128];
    if(pageAmount == 0 || nodeAmount == 0){
        printf("Invalid parameters, include -N nodeAmount and -M memoryAmount\n");
        exit(1);
    }
    printf("Server waiting for connections...\n");
    fflush(stdout);
    nodePages = (int) pageAmount/nodeAmount;
    for (int i = 0; i<pageAmount; i++){
        pageTable[i] = 1;
    }

    //Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    my_addr.sin_family = AF_INET;         /* host byte order */
    my_addr.sin_port = htons(PORT);     /* short, network byte order */
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* auto-fill with my IP */
    bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, QUEUE_SIZE) == -1) {
        perror("listen");
        exit(1);
    }
  
    while(1){
        //Opening connection
        newFd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);
        if(newFd == -1){
            break;
        }
        openSockets[connected] = newFd;
        int threadResult = pthread_create(&threads[connected], NULL, threadHandler,(void*)&newFd);
        if(threadResult != 0){
            printLog("ERROR",strerror(errno));
        }
        if(connected<=nodeAmount)
            connected++;
        if(connected == nodeAmount)
            initializeNodes();
    }   
}
