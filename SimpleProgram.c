#include "VirtualLibrary.h"

void readResponse();

ProgramInformation info;
int main(){
    printf("Setting up client connection...\n");   
    
    info = setupProgram();
    int addresses[200];
    int x_dir = allocate(info, sizeof(int));
    int * x_pointer = (int *) accessMemory(info,x_dir, WRITE_FLAG);
    accessMemory(info,x_dir, 0);
    * x_pointer = 4;
    printf("%d\n", * x_pointer);
    int x_dir2 = allocate(info,sizeof(int));
    int * x_pointer2 = (int *) accessMemory(info,x_dir2, WRITE_FLAG);
    accessMemory(info,x_dir2, 0);
    * x_pointer2 = 8;
    printf("%d\n", * x_pointer2);
    accessMemory(info,x_dir2, 0);
    accessMemory(info,x_dir, 0);
    deallocate(info,x_dir);
    int * x_pointer3;
    for(int ii = 0; ii < 20; ii++){
        addresses[ii] = allocate(info,2049);
        x_pointer3 = (int *) accessMemory(info,addresses[ii], WRITE_FLAG);
        * x_pointer3 = ii;
    }
    int total = 0;
    for(int ii = 0; ii < 20; ii = ii + 2){
        x_pointer3 = (int *) accessMemory(info,addresses[ii], READ_FLAG);
        total = total + * x_pointer3;
    }

    printf("Suma total de punteros (90)?: %d\n", total);

    x_dir = allocate(info,sizeof(int));
    accessMemory(info,x_dir, READ_FLAG);
    accessMemory(info,x_dir2, READ_FLAG);
    * x_pointer = 2;
    printf("%d\n", (*x_pointer) + (*x_pointer2));

    closeProgram(info);
    return 0;
}

