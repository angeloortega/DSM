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

typedef enum {LOG=0} resource;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};

struct table_entry
{
    int residing_node;
    int valid;
    time_t last_used;
};

static int semaphores[1];
static int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
static int last_fd;	/* Thelast sockfd that is connected	*/
static int node_amount;
static long memory_amount;
static char *logFile;


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

	for(int i = 0; i < 5 ; i++){
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
	for(int i = 0; i < 5 ; i++){
		random = rand()%8999+1000;
		semaphores[(int) i] = semget((key_t) random,1, 0666 | IPC_CREAT);
		if (semctl(semaphores[i], 0, SETVAL, sem_union) == -1)
			value = 0;
	}
	return(value);
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

void parseRequest(char* result[],char *request){
    const char * curLine = request;
    char type[3];
    int i = 0;
    if(request && request+1){
        type[0] = *request;
        type[1] = *(request+1);
        type[2] = '\0';
    }
    result[0] = type;
    result[1] = malloc(11);
    request[2] = malloc(11);
    request = request + 3;
    
    while(request && *request != "\r"){
            result[1][i] = *request; 
            request++;
            i++;
    }

    result[1][i] = '\0';
    if(strcmp(type,"01") == 0){
        request= request + 2;
        i= 0;
        while(request && *request != "\r"){
            result[2][i] = *request; 
            request++;
            i++;
        }
        result[2][i] = '\0';
    }

} 


void *clientHandler(void *arg){
    int n;
	char buffer[BUFFER_SIZE];
	char *request[3];
	//Read from client
    int fd = *((int*)arg); 
    if(recv(fd,buffer,sizeof(buffer),0) == -1)
        serverLog("ERROR",strerror(errno));    
    
    parseRequest(request, buffer);

    if(strcmp(request[0],"01") == 0){
        //Swap
        free(request[2]);
    }
    else{
        if(strcmp(request[0],"02") == 0){
        //Close

        }
        else{
            //Unsupported request
        }
    }
        
	free(request[0]);
	free(request[1]);
	pthread_exit(0);
}

int main(int argc, char* argv[]){
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
                    node_amount = node_amount * 10 + (nodes[i] - '0');
                }
            }
        }
        if(strcmp(argv[i], "-M") == 0){
            if(i + 1 < argc){
                char *nodes = argv[i+1];
                int length = strlen(nodes);
                memory_amount = 0l;
                for(int j=0; j < length; j++){
                    memory_amount = memory_amount * 10l + (nodes[i] - '0');
                }
            }
        }
    }
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    int sin_size;
    char buffer[BUFFER_SIZE];
    int	i;
	srand(time(NULL));

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

    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
        perror("accept");
        }
    fcntl(last_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/
    fcntl(new_fd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

    
    while(1){
        for (i=sockfd;i<=last_fd;i++){
            printf("Round number %d\n",i);
                if (i = sockfd){
                sin_size = sizeof(struct sockaddr_in);
                    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
                        perror("accept");
                    }
                    printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr)); 
                    fcntl(new_fd, F_SETFL, O_NONBLOCK);
                last_fd = new_fd;
            }
            else{
            
                pthread_t thread;
                int threadResult = pthread_create(&thread, NULL, clientHandler,new_fd);
                if(threadResult != 0){
                    serverLog("ERROR",strerror(errno));
                }
    
        }
    }

        
}
