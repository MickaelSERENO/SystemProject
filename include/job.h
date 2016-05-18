#ifndef  JOB_INC
#define  JOB_INC

#include "global.h"
#include "Child.h"

void childThread(char* path, char** argv);
void split(const char* buffer, char** argv, char splitChar);
void handle_int(int num);
void handle_tstp(int num);
void* getStdoutPipe(void* pipe);
void* getStderrPipe(void* pipe);
void waitChild(uint32_t jID, uint32_t cID);

void Job_kill(uint32_t jID, int sig);
void cleanJob();

#endif   /* ----- #ifndef JOB_INC  ----- */
