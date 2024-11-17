#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <signal.h>

#define drone 0        
#define input 1        
#define obstacle 2        
#define target 3 
#define blackboard 4

int writeSecure(char* filename, char* data, int numeroRiga, char mode);
int readSecure(char* filename, char* data, int numeroRiga);
void handler(int id, int pid, int sleep);

#endif