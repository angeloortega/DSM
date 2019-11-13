#include <sys/types.h>
 
int DSM_node_init (int *argc, char **argv[], int *nodes, int *nid);

void *DSM_node_exit (void);

void *DSM_malloc (size_t size);

void DSM_barrier (void);
