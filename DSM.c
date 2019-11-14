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



typedef enum {LOG=0, TABLE=1} resource;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};

static int semaphores[2];
static int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
static int last_fd;	/* Thelast sockfd that is connected	*/
static int node_amount;
static int page_amount;
static int memory_amount;
static char *logFile;
static int pages_per_node; 
int* valid;
int* node_sockets;
int connected;

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
		semaphores[(int) i] = semget((key_t) random,1, 0666 | IPC_CREAT);
		if (semctl(semaphores[i], 0, SETVAL, sem_union) == -1)
			value = 0;
	}
	return(value);
}

void pageUsage(int page){
    valid[page] = 0;
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
		printf("%d:%d:%d - %s: %s\n",  (*startTime).tm_hour,  (*startTime).tm_min,  (*startTime).tm_sec, type, message);
		fprintf(f, "%d:%d:%d - %s: %s",  (*startTime).tm_hour, (*startTime).tm_min,  (*startTime).tm_sec, type, message);
		fclose(f);

	}
	if (semaphore_v(LOG)==-1)
		exit(EXIT_FAILURE);
}
void closeServer(){

    //Close server and clean up sockets and memory
    for (int i=sockfd;i<=last_fd;i++){
        close(i);
    }
    close(sockfd);
    printf("Closing server...\n");
    serverLog("STATUS","server shutting down");
    del_semvalue();
}

int nodePages(int node, int nodes, int pages){
    return  node >= pages % nodes ? (int) pages/nodes: (int) pages/nodes + 1;
}

void initiallizeNodes(){
    for(int i = 0; i < node_amount; i++){
        DSM_node_pages(node_sockets[i], nodePages(i, node_amount, page_amount));
    }
}

void *clientHandler(void *arg){
    int n;
    char buffer[BUFFER_SIZE];
    char *request[2];
    int fd = *((int*)arg);
    while(1){
        //Read from client
        if(recv(fd,buffer,sizeof(buffer),0) == -1)
            serverLog("ERROR",strerror(errno));    
        
        parseRequest(request, buffer);
        /*
        #define INIT_MESSAGE "00\r\n%d\r\n\r\n"
        #define WRITE_MESSAGE "01\r\n%d\r\n\r\n"
        #define READ_MESSAGE "02\r\n%d\r\n\r\n"
        #define CLOSE_MESSAGE "03\r\n%d\r\n\r\n"
        #define INVALIDATE_MESSAGE "04\r\n%d\r\n\r\n"
        */
        if(strcmp(request[0],"01") == 0){
            //write
            char *result = strstr(buffer, "\r\n\r\n");
            void *temp_page;
            result = result + 4;
            int page = atoi(request[1]);
            memcpy(temp_page,result,(size_t) PAGE_SIZE); 
            DSM_page_write(node_sockets[page%node_amount],(int) page/node_amount, temp_page);
        }
        else{
            if(strcmp(request[0],"02") == 0){
                void *temp_page;
                int page = atoi(request[1]);
                temp_page = DSM_page_read(node_sockets[page%node_amount],(int) page/node_amount);
                write(fd,temp_page, PAGE_SIZE);
                valid[page] = 1;
            }
            
            else{
                if(strcmp(request[0],"03") == 0){
                    //CLOSE SOCKET TODO
                // pageUsage(atoi(request[1]));
                }
                else{
                    if(strcmp(request[0],"04") == 0){
                        int page = atoi(request[1]);
                        valid[page] = 0;
                    }
                    //Unsupported request
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
    //Virtual address table creation
    page_amount = (memory_amount/PAGE_SIZE) + ( memory_amount % PAGE_SIZE == 0 ? 0 : 1);

    valid = malloc(sizeof(int) * page_amount);
    node_sockets = malloc(sizeof(int) * page_amount);
    printf("page amount:%d node amount:%d",page_amount,node_amount);
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
    last_fd = sockfd;

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
/*
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
        perror("accept");
        }
*/ 
    //fcntl(last_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

    
    while(1){
        //Opening connection
        pthread_t thread;
        new_fd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);
        //fcntl(new_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/
        node_sockets[connected++] = new_fd;
        int threadResult = pthread_create(&thread, NULL, clientHandler,(void*)&new_fd);
        if(threadResult != 0){
            serverLog("ERROR",strerror(errno));
        }
        if(connected == node_amount)
            initiallizeNodes();
    }   
}
