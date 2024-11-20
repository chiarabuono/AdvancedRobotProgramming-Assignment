#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <signal.h>

#define DRONE 0        
#define INPUT 1        
#define OBSTACLE 2        
#define TARGET 3 
#define BLACKBOARD 4

int writeSecure(char* filename, char* data, int numeroRiga, char mode);
int readSecure(char* filename, char* data, int numeroRiga);
void handler(int id, int sleep);

#endif