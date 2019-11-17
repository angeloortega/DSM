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
#include <signal.h>

typedef enum {LOG=0, TABLE=1} resource;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};

static int semaphores[2];
static int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
static int node_amount;
static int page_amount;
static int memory_amount;
static char *logFile;
static int pages_per_node; 
int* valid;
int* node_sockets;
int connected;
pthread_t *threads;
static int semaphore_v(resource res){
	struct sembuf sem_b;
	sem_b.sem_num = 0; 
	sem_b.sem_op = 1; 
	sem_b.sem_flg = SEM_UNDO;
	int item = (int) res;
	if (semop(semaphores[item], &sem_b, 1) == -1) { 
		perror("semaphore_v failed\n"); 
		return(0); 
	} 
	return(1);
}

static int semaphore_p(resource res){
	struct sembuf sem_b;
	sem_b.sem_num = 0; 
	sem_b.sem_op = -1;
	sem_b.sem_flg = SEM_UNDO; 
	int item = (int) res;
	if (semop(semaphores[item], &sem_b, 1) == -1) { 
		perror("semaphore_p failed\n"); 
		return(0); 
	} 
	return(1);
}

static void del_semvalue(void) // This removes the semaphore from the system
{ 
	union semun sem_union;

	for(int i = 0; i < 2 ; i++){
		if (semctl(semaphores[i], 0, IPC_RMID, sem_union) == -1) 
			perror("Failed to delete semaphore\n");
	}
}

static int set_semvalue(void) // This initializes the semaphore
{ 
	union semun sem_union;
	int value = 1;
	sem_union.val = 1; 
	int random = 0;
	for(int i = 0; i < 2 ; i++){
		random = rand()%8999+1000;
		semaphores[i] = semget((key_t) random,1, 0666 | IPC_CREAT);
		if (semctl(semaphores[i], 0, SETVAL, sem_union) == -1)
			value = 0;
	}
	return(value);
}

void serverLog(char* type, char* message){
	int flag;
	do{
		flag = semaphore_p(LOG);
		if(!flag){
			sleep(1);
		}
	}
	while(!flag);
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
		printf("%02d:%02d:%02d - %s: %s\n",  (*startTime).tm_hour,  (*startTime).tm_min,  (*startTime).tm_sec, type, message);
		fflush(stdout);
        fprintf(f, "%02d:%02d:%02d - %s: %s",  (*startTime).tm_hour, (*startTime).tm_min,  (*startTime).tm_sec, type, message);
		fclose(f);

	}
	if (semaphore_v(LOG)==-1)
		exit(EXIT_FAILURE);
}
void closeServer(){

    //Close server and clean up sockets and memory
    if(getpid() == getpid()){  // Only running on first thread
        for (int i=0;i<=connected;i++){
            pthread_cancel(threads[i]);
            close(node_sockets[i]);
        }
        close(sockfd);
        serverLog("STATUS","server shutting down");
        del_semvalue();
        free(node_sockets);
        free(valid);
    }
}

void exitSignal(int sig)
{
    serverLog("STATUS","\nProgram interrupted...\n");
	closeServer();
}

int nodePages(int node, int nodes, int pages){
    return  node >= pages % nodes ? (int) pages/nodes: (int) pages/nodes + 1;
}

void printPages(int socket){
    printf("Memory map for node #%d: (Each cell contains the virtual memory number)\n", socket);
    int amount = nodePages(socket,node_amount,page_amount);
    for(int j = 0; j < amount; j++){
        if(j%5 == 0)
            printf("\n");
        printf("[%05d]", socket + j*node_amount);
    }
    printf("\n");
}

int findNode(int fd){
    for(int i = 0; i<node_amount; i++){
        if(node_sockets[i] == fd){
            return i;
        }
    }
    return -1;
}

void initializeNodes(){
    for(int i = 0; i < node_amount; i++){
        DSM_node_pages(node_sockets[i], nodePages(i, node_amount, page_amount));
    }
}

void *clientHandler(void *arg){
    int n;
    char buffer[BUFFER_SIZE];
    char *request[3];
    int fd = *((int*)arg);
    int readAmount = 0;
    char message[128];
    while(1){
        //Read from client
        readAmount = read(fd,&buffer,(size_t)BUFFER_SIZE);
        if(readAmount == -1){
             if(errno==EINTR || errno==9){
                printf("Interrupted thread\n");
                fflush(stdout);
                pthread_exit((void*) 1);
                break;
            }
            serverLog("ERROR",strerror(errno));
            continue;    
        }
        if(readAmount == 0 || *buffer == '\0'){
            close(fd);
            break;    
        }    
        sprintf(message,"received %d bytes from %d\n",readAmount, fd);
        serverLog("THREAD",message);
        parseRequest(request, buffer);
        if(strcmp(request[0],"00") == 0){
            //Node successfully created
            sprintf(message,"Node %d was able to reserve %s pages of %d bytes\n",fd, request[1], PAGE_SIZE);
            printPages(findNode(fd));
            serverLog("THREAD",message);
            fflush(stdout); 
        }
        else{
            if(strcmp(request[0],"01") == 0){
                //write
                char *result = strstr(buffer, "\r\n\r\n");
                void *temp_page = malloc(sizeof(char*) * PAGE_SIZE);
                int page = atoi(request[1]);

                if(page >= page_amount){
                    serverLog("ERROR","Attempting to reach out of bounds memory\n");       
                    free(temp_page);
                    continue;
                }
                int node =  node_sockets[page%node_amount];
                if(node == -1){
                    serverLog("ERROR","Attempting to reach closed node\n");
                    free(temp_page);
                    continue;
                }
                result = result + 4;
                memcpy(temp_page,result,(size_t) PAGE_SIZE);
                DSM_page_write(node, (int) page/node_amount, temp_page);
                free(temp_page);
                valid[page] = 1;
            }
            else{
                if(strcmp(request[0],"02") == 0){
                    int page = atoi(request[1]);

                    if(page >= page_amount){
                        serverLog("ERROR","Attempting to reach out of bounds memory\n");                               
                        continue;
                    }
                    int node =  node_sockets[page%node_amount];
                    if(node == -1){
                        serverLog("ERROR","Attempting to reach closed node\n");
                        continue;
                    }
                    int requester = findNode(fd);
                    DSM_page_read(node,(int) page/node_amount, requester);
                    free(request[2]);
                }
                
                else{
                    if(strcmp(request[0],"03") == 0){
                        sprintf(message,"Node %d closing down!\n",fd);
                        serverLog("ERROR",message);                               
                        int node = findNode(fd);
                        for(int i = 0; i < page_amount; i++){
                            valid[node_amount*i + node] = 0; //Invalidate all pages related to node
                        }
                        node_sockets[node] = -1;
                        close(fd);
                        free(request[0]);
                        free(request[1]);
                        break;
                    }
                    else{
                        if(strcmp(request[0],"04") == 0){
                            int page = atoi(request[1]);

                            if(page >= page_amount){
                                serverLog("ERROR","Attempting to reach out of bounds memory\n");                               
                                continue;
                            }   
                            valid[page] = 0;
                        }

                        else{
                            if(strcmp(request[0],"05") == 0){
                                //write
                                char *result = strstr(buffer, "\r\n\r\n");
                                void *temp_page = malloc(sizeof(char*) * PAGE_SIZE);
                                result = result + 4;
                                int page = atoi(request[1]);

                                if(page >= page_amount){
                                    serverLog("ERROR","Attempting to reach out of bounds memory\n");                               
                                    free(temp_page);
                                    free(request[2]);
                                    continue;
                                }

                                int source = findNode(fd);
                                int destination = node_sockets[atoi(request[2])];
                                if(destination == -1){
                                    serverLog("ERROR","Attempting to reach] closed node\n");                               
                                    free(temp_page);
                                    free(request[2]);
                                    continue;
                                }
                                memcpy(temp_page,result,(size_t) PAGE_SIZE); 
                                DSM_page_read_response(destination,(int) page*node_amount + source, temp_page,destination);
                                free(temp_page);
                                free(request[2]);
                            }
                        //Unsupported request
                        }
                    }
                }
            }
        }
        free(request[0]);
        free(request[1]);
    }
	pthread_exit(0);
}

int main(int argc, char* argv[]){
    node_amount = 0;
    memory_amount = 0;
    for(int i=0; i<argc; ++i){   
        if(strcmp(argv[i], "-L") == 0){
            if(i + 1 < argc){
                logFile = argv[i+1];
            }
        }
        if(strcmp(argv[i], "-N") == 0){
            if(i + 1 < argc){
                char *nodes = argv[i+1];
                int length = strlen(nodes);
                node_amount = 0;
                for(int j=0; j < length; j++){
                    node_amount = node_amount * 10 + (nodes[j] - '0');
                }
            }
        }
        if(strcmp(argv[i], "-M") == 0){
            if(i + 1 < argc){
                char *nodes = argv[i+1];
                int length = strlen(nodes);
                memory_amount = 0;
                for(int j=0; j < length; j++){
                    memory_amount = memory_amount * 10 + (nodes[j] - '0');
                }
            }
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
    page_amount = (memory_amount/PAGE_SIZE) + ( memory_amount % PAGE_SIZE == 0 ? 0 : 1);
    valid = malloc(sizeof(int) * page_amount);
    node_sockets = malloc(sizeof(int) * node_amount);
    threads = malloc(sizeof(pthread_t) * node_amount);
    char message[128];
    if(page_amount == 0 || node_amount == 0){
        printf("Invalid parameters, include -N node_amount and -M memory_amount\n");
        exit(1);
    }
    sprintf(message, "page amount:%d node amount:%d\n",page_amount,node_amount);
    serverLog("INFO",message);                               
    fflush(stdout);
    pages_per_node = (int) page_amount/node_amount;
    for (int i = 0; i<page_amount; i++){
        valid[i] = 1;
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

	if (!set_semvalue()) { 
        serverLog("ERROR", "Failed to initialize semaphores"); 
        exit(EXIT_FAILURE); 
    }

    if (listen(sockfd, QUEUE_SIZE) == -1) {
        perror("listen");
        exit(1);
    }
  
    while(1){
        //Opening connection
        new_fd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);
        if(new_fd == -1){
            break;
        }
        //fcntl(new_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/
        node_sockets[connected] = new_fd;
        int threadResult = pthread_create(&threads[connected], NULL, clientHandler,(void*)&new_fd);
        if(threadResult != 0){
            serverLog("ERROR",strerror(errno));
        }
        connected++;
        if(connected == node_amount)
            initializeNodes();
    }   
}
