#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <signal.h>

#define DRONE 0        
#define INPUT 1        
#define OBSTACLE 2        
#define TARGET 3 
#define BLACKBOARD 4

#define WINDOW_WIDTH 100
#define WINDOW_LENGTH 100

#define NUM_TARGET 5
#define NUM_OBSTACLES 10
#define NO_SPAWN_DIST 5
#define ETA 0.2

#define FORCE_THRESHOLD 5 //[m]

typedef struct {
    int x;
    int y;
} Drone_bb;


typedef struct {
    float x;
    float y;
} Force;

int writeSecure(char* filename, char* data, int numeroRiga, char mode);
int readSecure(char* filename, char* data, int numeroRiga);
void handler(int id, int sleep);

void fromStringtoDrone(Drone_bb *drone, const char *drone_str, FILE *file);
void fromStringtoForce(Force *force, const char *force_str, FILE *file);

#endif