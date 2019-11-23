#include "Declarations.h"
void exitConnection(int socket_fd);

int beginConnection(void);

void readPage(int socket_fd,int page, int recipient);

void returnPage(int socket_fd,int page, void * body, int recipient);

void writePage(int socket_fd,int page, void * body);

void sendMessage(int socket_fd,int page, char* message);

void parseRequest(char* result[],char *request);

void readResponse(ProgramInformation info);

char* accessMemory(ProgramInformation info, int address, int writeFlag);

int allocate(ProgramInformation info,int bytes);

void deallocate(ProgramInformation info,int address);

ProgramInformation setupProgram();

void closeProgram(ProgramInformation info);

