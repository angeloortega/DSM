#ifndef COMMON_INCLUDE_FILE
#define COMMON_INCLUDE_FILE
#define PORT 8080
#define IP "127.0.0.1"
#define BUFFER_SIZE 2048 //525312
#define QUEUE_SIZE 25
#define PAGE_SIZE 4096//524288
#define INIT_MESSAGE "00\r\n%d\r\n\r\n"
#define WRITE_MESSAGE "01\r\n%d\r\n\r\n"
#define READ_MESSAGE "02\r\n%d\r\n%d\r\n\r\n" //Read local page n, send it to m
#define CLOSE_MESSAGE "03\r\n%d\r\n\r\n"
#define INVALIDATE_MESSAGE "04\r\n%d\r\n\r\n"
#define READ_RESPONSE "05\r\n%d\r\n%d\r\n\r\n" //Read local page n, send it to m
#endif
