#include "VirtualLibrary.h"

void readResponse();

ProgramInformation info;
int main(){
    printf("Setting up client connection...\n");   
    
    info = setupProgram();
    
    int x_dir = allocate(info, sizeof(int));
    int * x_pointer = (int *) accessMemory(info,x_dir, 1);
    accessMemory(info,x_dir, 0);
    * x_pointer = 4;
    printf("%d\n", * x_pointer);
    int x_dir2 = allocate(info,sizeof(int));
    int * x_pointer2 = (int *) accessMemory(info,x_dir2, 1);
    accessMemory(info,x_dir2, 0);
    * x_pointer2 = 8;
    printf("%d\n", * x_pointer2);
    accessMemory(info,x_dir2, 0);
    accessMemory(info,x_dir, 0);
    deallocate(info,x_dir);
    x_dir = allocate(info,sizeof(int));

    for(int ii = 0; ii < 20; ii++){
        accessMemory(info,allocate(info,2049), 0);
    }
    accessMemory(info,x_dir, 0);
    accessMemory(info,x_dir2, 0);
    * x_pointer = 2;
    printf("%d\n", (*x_pointer) + (*x_pointer2));

    closeProgram(info);
    return 0;
}

