#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h> 
#include <semaphore.h>
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#define PORT 8080
#define IP "127.0.0.1"
#define BUFFER_SIZE 4352
#define QUEUE_SIZE 10
#define PAGE_SIZE 4096
#define WRITE_FLAG 1
#define READ_FLAG 1
#define BEGIN_CONNECTION "00\r\n%d\r\n\r\n"
#define WRITE_PAGE "01\r\n%d\r\n\r\n"
#define READ_PAGE "02\r\n%d\r\n%d\r\n\r\n"
#define CLOSE_CONNECTION "03\r\n%d\r\n\r\n"
#define INVALIDATE_PAGE "04\r\n%d\r\n\r\n"
#define READ_RESULT "05\r\n%d\r\n%d\r\n\r\n" 
#define WRITE_RESULT "06\r\n%d\r\n\r\n"
struct Item{
    int beginning;
    int end;
    struct Item* next;
};
typedef struct Item * Item;

struct Page{
    Item busyMemory;
};
typedef struct Page * Page;

struct ProgramInformation{
    char **localMemory;
    int *pageBuffer;
    int *pageValid;
    Page pageInfo;
    int fd;
    int pageAmount;
    int nodeAmount; 
    int pagesPerNode;
};
typedef struct ProgramInformation * ProgramInformation;
