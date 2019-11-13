#include <sys/types.h> 

void DSM_node_exit(int socket_fd);

int DSM_malloc (int socket_fd, size_t size);

int DSM_node_init(void);

char * DSM_page_swap(int socket_fd,int sent, int received, char * body);
