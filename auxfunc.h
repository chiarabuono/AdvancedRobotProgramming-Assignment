#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <signal.h>

#define DRONE 0        
#define INPUT 1        
#define OBSTACLE 2        
#define TARGET 3 
#define BLACKBOARD 4

#define WINDOW_WIDTH 99
#define WINDOW_LENGTH 99

#define MAX_TARGET 5
#define MAX_OBSTACLES 10
#define NO_SPAWN_DIST 5
#define ETA 0.2

#define FORCE_THRESHOLD 5 //[m]
#define MIN_THRESHOLD 2 //[m]

#define TARGET_DETECTION 1

#define len_str_targets 6 * MAX_TARGET + 2
#define len_str_obstacles 6 * MAX_OBSTACLES + 2

extern const char *moves[9];

extern int numTarget;
extern int numObstacle;

typedef struct {
    int x;
    int y;
} Drone_bb;


typedef struct {
    float x;
    float y;
} Force;

typedef struct {
    int x[MAX_TARGET];
    int y[MAX_TARGET];
    int value[MAX_TARGET];
} Targets;

typedef struct
{
    int x[MAX_OBSTACLES];
    int y[MAX_OBSTACLES];
} Obstacles;

typedef struct {
    float x;
    float y;
} Speed;

int writeSecure(char* filename, char* data, int numeroRiga, char mode);
int readSecure(char* filename, char* data, int numeroRiga);
void handler(int id, int sleep);

void fromStringtoDrone(Drone_bb *drone, const char *drone_str, FILE *file);
void fromStringtoForce(Force *force, const char *force_str, FILE *file);
void fromStringtoPositions(Drone_bb *drone, int *x, int *y, const char *str, FILE *file);

void fromStringtoPositionsWithTwoTargets(int *x1, int *y1, int *x2, int *y2, const char *str, FILE *file);


void fromPositiontoString(int *x, int *y, int len, char *str, size_t str_size, FILE *file);
void concatenateStr(const char *str1, const char *str2, char *output, size_t output_size, FILE *file);
void droneInfotoString(Drone_bb *drone, Force *force, Speed *speed, char *output, size_t output_size, FILE *file);
void fromStringtoDroneInfo(char *input_str, char *drone_str);
#endif