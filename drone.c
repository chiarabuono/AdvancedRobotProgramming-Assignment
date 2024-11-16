#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "auxfunc.h"
#include <math.h>
#include <sys/select.h>

//#include <errno.h>

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define MAX_DIRECTIONS 80
#define WINDOW_WIDTH 400
#define WINDOW_LENGHT 800
#define PERIOD 10 //[Hz]

typedef struct {
    float x;
    float y;
    float previous_x[2];   // 0 is one before and 1 is is two before
    float previous_y[2];

} Element;

typedef struct {
    int x;
    int y;
} Element_bb;

const char* moves[] = {"up", "down", "right", "left", "upleft", "upright", "downleft", "downright"};
Element drone = {10.0, 20.0, 11.0, 21.0, 12.0, 22.0};  

// Useful for debugging
void printPosition(Element p) {
    printf("Position: (x = %f, y = %f)\n", p.x, p.y);
    printf("BB Pos: (x = %.0f, y = %.0f)\n", round(p.x), round(p.y));
}

// Simulate user input
void fillDirections(const char* filled[], int* size) {
    for (int i = 0; i < *size; i++) {
        int randomIndex = 7;
        filled[i] = moves[randomIndex]; 
    }
}

// Update drone position
void updatePosition(Element* p, const char* direction, float* force) {

    // store current position and slide the previous position to the pre-previous position
    p->previous_x[1] = p->previous_x[0];
    p->previous_y[1] = p->previous_y[0];
    p->previous_x[0] = p->x;
    p->previous_y[0] = p->y;

    // conditions to stay inside the window
    int up_condition = (p->y > 1) ? 1 : 0;
    int down_condition = (p->y < WINDOW_WIDTH - 1) ? 1 : 0;
    int right_condition = (p->x < WINDOW_LENGHT - 1) ? 1 : 0;
    int left_condition = (p->x > 1) ? 1 : 0;


    if (strcmp(direction, "up") == 0 && up_condition) p->y -= force[0];
    else if (strcmp(direction, "down") == 0 && down_condition) p->y += force[0];
    else if (strcmp(direction, "right") == 0 && right_condition) p->x += force[0];
    else if (strcmp(direction, "left") == 0 && left_condition) p->x -= force[0];

    else if (strcmp(direction, "upleft") == 0 && up_condition && left_condition) {
        p->x -= force[0] / sqrt(2);
        p->y -= force[0] / sqrt(2);
        }
    else if (strcmp(direction, "upright") == 0 && up_condition && right_condition) {
        p->x += force[0] / sqrt(2);
        p->y -= force[0] / sqrt(2);
    }
    else if (strcmp(direction, "downleft") == 0 && down_condition && left_condition) {
        p->x -= force[0] / sqrt(2);
        p->y += force[0] / sqrt(2);
    }
    else if (strcmp(direction, "downright") == 0 && down_condition && right_condition) {
        p->x += force[0] / sqrt(2);
        p->y += force[0] / sqrt(2);
    }

}
// Remove first element of the array
void removeFirstElement(const char* directions[], int* size) {
    if (*size == 0) {
        *size = MAX_DIRECTIONS;
        printPosition(drone);
        fillDirections(directions, size);  // Refill the array if empty
    } else {
        for (int i = 1; i < *size; ++i) {
            directions[i - 1] = directions[i]; 
        }
        (*size)--;
    }
}

float* drone_force(Element* p, float mass, float K) {

    float* force = (float*)malloc(2 * sizeof(float));
    if (force == NULL) return NULL;  // Return NULL if memory allocation fails

    float derivative1x = (p->x - p->previous_x[0])/PERIOD;
    float derivative2x = (p->previous_x[1] + p->x -2 * p->previous_x[0])/(PERIOD * PERIOD);
    force[0] = (mass * derivative2x + K * derivative1x == 0) ? 1: mass * derivative2x + K * derivative1x;

    float derivative1y = (p->y - p->previous_y[0])/PERIOD;
    float derivative2y = (p->previous_y[1] + p->y -2 * p->previous_y[0])/(PERIOD * PERIOD);
    force[1] = (mass * derivative2y + K * derivative1y == 0) ? 1: mass * derivative2y + K * derivative1y;  

    return force;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    FILE *file = fopen("outputobstacle.txt", "a");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    // FDs reading
    char *fd_str = argv[1];
    int fds[4]; 
    int index = 0;
    
    char *token = strtok(fd_str, ",");
    token = strtok(NULL, ","); 

    // FDs extraction
    while (token != NULL && index < 4) {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }

    int pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "d%d,", pid);
    
    if(writeSecure("log.txt", dataWrite) == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    const char* directions[MAX_DIRECTIONS];
    int directionCount = MAX_DIRECTIONS;

    // Simulate user input
    fillDirections(directions, &directionCount);

    //for (int i = 0; i < MAX_DIRECTIONS; i++) printf("%s\n", directions[i]);
    float mass = 1.0;
    float K = 1.0;
    float* F;


    while (1) {
        F = drone_force(&drone, mass, K);
        
        updatePosition(&drone, directions[0], F);
        if (drone.x < 0 || drone.y < 0 || drone.x > WINDOW_LENGHT || drone.y > WINDOW_WIDTH) printf("ERROR");

        //printf("Removing direction: %s\n", directions[0]);
        removeFirstElement(directions, &directionCount);

        //printf("F = (%f, %f)\n", F[0], F[1]);
        sleep(1/PERIOD); 
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
