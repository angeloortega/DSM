void DSM_node_exit(int socket_fd);

int DSM_node_init(void);

int DSM_node_pages (int socket_fd, int amount);

void * DSM_page_read(int socket_fd,int page);

void DSM_page_write(int socket_fd,int page, void * body);

void DSM_page_invalidate(int socket_fd,int page);

void parseRequest(char* result[],char *request);