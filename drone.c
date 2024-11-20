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
#define WINDOW_WIDTH 100
#define WINDOW_LENGTH 100
#define PERIOD 100 //[Hz]
#define NUM_OBSTACLES 10
#define NUM_TARGET 5

// drone
# define DRONEMASS 1


// management obstacles
int obstacle_threshold = 5; //[m]
int obstacle_mass = 0.1;
#define NO_SPAWN_DIST 5
#define ETA 0.2

// management target
#define MAX_TARGET_VALUE 9

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

typedef struct {
    float x[NUM_OBSTACLES];
    float y[NUM_OBSTACLES];
} Obstacles;

typedef struct {
    float x[NUM_TARGET];
    float y[NUM_TARGET];
    int value[NUM_TARGET];
} Targets;

const char* moves[] = {"up", "down", "right", "left", "upleft", "upright", "downleft", "downright"};
Element drone = {10.0, 20.0, {10.0, 20.0}, {10.0, 20.0}};  

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

Obstacles createObstacles(Element drone) {
    Obstacles obstacles;
    float x_pos, y_pos;

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        do {
            x_pos = rand() % WINDOW_LENGTH;
            y_pos = rand() % WINDOW_WIDTH;
        } while (
            (x_pos >= drone.x - NO_SPAWN_DIST && x_pos <= drone.x + NO_SPAWN_DIST) &&
            (y_pos >= drone.y - NO_SPAWN_DIST && y_pos <= drone.y + NO_SPAWN_DIST)
        );

        obstacles.x[i] = x_pos;
        obstacles.y[i] = y_pos;
    }
    return obstacles;
}

// Simulate obstacle moving
void obstaclesMoving(Obstacles obstacles) {
    int num_moves = sizeof(moves) / sizeof(moves[0]);
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        const char* move = moves[rand() % num_moves];

        int up_condition = (obstacles.y[i] > 1);
        int down_condition = (obstacles.y[i] < WINDOW_WIDTH - 1);
        int right_condition = (obstacles.x[i] < WINDOW_LENGTH - 1);
        int left_condition = (obstacles.x[i] > 1);

        if (strcmp(move, "up") == 0 && up_condition) obstacles.y[i] -= 1;
        else if (strcmp(move, "down") == 0 && down_condition) obstacles.y[i] += 1;
        else if (strcmp(move, "right") == 0 && right_condition) obstacles.x[i] += 1;
        else if (strcmp(move, "left") == 0 && left_condition) obstacles.x[i] -= 1;

        else if (strcmp(move, "upleft") == 0 && up_condition && left_condition) {
            obstacles.x[i] -= (float)1.0 / sqrt(2);
            obstacles.y[i] -= (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "upright") == 0 && up_condition && right_condition) {
            obstacles.x[i] += (float)1.0 / sqrt(2);
            obstacles.y[i] -= (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "downleft") == 0 && down_condition && left_condition) {
            obstacles.x[i] -= (float)1.0 / sqrt(2);
            obstacles.y[i] += (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "downright") == 0 && down_condition && right_condition) {
            obstacles.x[i] += (float)1.0 / sqrt(2);
            obstacles.y[i] += (float)1.0 / sqrt(2);
        }
    }
}

Targets createTargets(Element drone) {
    Targets targets;
    int x_pos, y_pos;

    for (int i = 0; i < NUM_TARGET; i++) {

        do {
            x_pos = rand() % WINDOW_LENGTH;
            y_pos = rand() % WINDOW_WIDTH;

        } while (
            (x_pos >= drone.x - NO_SPAWN_DIST && x_pos <= drone.x + NO_SPAWN_DIST) &&
            (y_pos >= drone.y - NO_SPAWN_DIST && y_pos <= drone.y + NO_SPAWN_DIST)
        );
        targets.value[i] = rand() % MAX_TARGET_VALUE;
        targets.x[i] = x_pos;
        targets.y[i] = y_pos;
    }
    return targets;
}

void targetsMoving(Targets targets) {
    int num_moves = sizeof(moves) / sizeof(moves[0]);
    for (int i = 0; i < NUM_TARGET; i++) {
        const char* move = moves[rand() % num_moves];

        int up_condition = (targets.y[i] > 1);
        int down_condition = (targets.y[i] < WINDOW_WIDTH - 1);
        int right_condition = (targets.x[i] < WINDOW_LENGTH - 1);
        int left_condition = (targets.x[i] > 1);

        if (strcmp(move, "up") == 0 && up_condition) targets.y[i] -= 1;
        else if (strcmp(move, "down") == 0 && down_condition) targets.y[i] += 1;
        else if (strcmp(move, "right") == 0 && right_condition) targets.x[i] += 1;
        else if (strcmp(move, "left") == 0 && left_condition) targets.x[i] -= 1;

        else if (strcmp(move, "upleft") == 0 && up_condition && left_condition) {
            targets.x[i] -= (float)1.0 / sqrt(2);
            targets.y[i] -= (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "upright") == 0 && up_condition && right_condition) {
            targets.x[i] += (float)1.0 / sqrt(2);
            targets.y[i] -= (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "downleft") == 0 && down_condition && left_condition) {
            targets.x[i] -= (float)1.0 / sqrt(2);
            targets.y[i] += (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "downright") == 0 && down_condition && right_condition) {
            targets.x[i] += (float)1.0 / sqrt(2);
            targets.y[i] += (float)1.0 / sqrt(2);
        }
    }
}

typedef struct {
    float x;
    float y;
} Force;

Force obstacle_force(Element* drone, Obstacles obstacles) {
    Force force = {0, 0};
    float deltaX, deltaY, distance, distance2;

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        deltaX = drone->x - obstacles.x[i];
        deltaY = drone->y - obstacles.y[i];
        distance2 = pow(deltaX, 2) + pow(deltaY, 2);

        if (distance2 == 0 || distance2 > pow(obstacle_threshold, 2)) continue; // Beyond influence radius

        distance = sqrt(distance2);

        float repulsion = ETA * pow((1 / distance - 1 / obstacle_threshold), 2) / distance;
        force.x += repulsion * (deltaX / distance); // Normalize direction
        force.y += repulsion * (deltaY / distance);
    }

    return force;
}

Force target_force(Element* drone, Targets targets) {
    Force force = {0, 0};
    float deltaX, deltaY, distance, distance2;

    for (int i = 0; i < NUM_TARGET; i++) {
        deltaX = targets.x[i] - drone->x;
        deltaY = targets.y[i] - drone->y;
        distance2 = pow(deltaX, 2) + pow(deltaY, 2);

        distance = sqrt(distance2);

        if (distance < obstacle_threshold) continue; // Ignore very close targets

        float attraction = ETA * distance; // Linear or quadratic attraction
        force.x += attraction * (deltaX / distance);
        force.y += attraction * (deltaY / distance);
    }

    return force;
}

// Update drone position
void updatePosition(Element* p, const char* direction, Force force) {

    // store current position and slide the previous position to the pre-previous position
    p->previous_x[1] = p->previous_x[0];
    p->previous_y[1] = p->previous_y[0];
    p->previous_x[0] = p->x;
    p->previous_y[0] = p->y;

    // conditions to stay inside the window
    int up_condition = (p->y > 1) ? 1 : 0;
    int down_condition = (p->y < WINDOW_WIDTH - 1) ? 1 : 0;
    int right_condition = (p->x < WINDOW_LENGTH - 1) ? 1 : 0;
    int left_condition = (p->x > 1) ? 1 : 0;


    if (strcmp(direction, "up") == 0 && up_condition) p->y -= force.x;
    else if (strcmp(direction, "down") == 0 && down_condition) p->y += force.y;
    else if (strcmp(direction, "right") == 0 && right_condition) p->x += force.x;
    else if (strcmp(direction, "left") == 0 && left_condition) p->x -= force.y;

    else if (strcmp(direction, "upleft") == 0 && up_condition && left_condition) {
        p->x -= force.x / sqrt(2);
        p->y -= force.y / sqrt(2);
        }
    else if (strcmp(direction, "upright") == 0 && up_condition && right_condition) {
        p->x += force.x / sqrt(2);
        p->y -= force.y / sqrt(2);
    }
    else if (strcmp(direction, "downleft") == 0 && down_condition && left_condition) {
        p->x -= force.x / sqrt(2);
        p->y += force.y / sqrt(2);
    }
    else if (strcmp(direction, "downright") == 0 && down_condition && right_condition) {
        p->x += force.x / sqrt(2);
        p->y += force.y / sqrt(2);
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

Force drone_force(Element* p, float mass, float K) {

    Force force;

    float derivative1x = (p->x - p->previous_x[0])/PERIOD;
    float derivative2x = (p->previous_x[1] + p->x -2 * p->previous_x[0])/(PERIOD * PERIOD);
    force.x = (mass * derivative2x + K * derivative1x == 0) ? 1: mass * derivative2x + K * derivative1x;

    float derivative1y = (p->y - p->previous_y[0])/PERIOD;
    float derivative2y = (p->previous_y[1] + p->y -2 * p->previous_y[0])/(PERIOD * PERIOD);
    force.y = (mass * derivative2y + K * derivative1y == 0) ? 1: mass * derivative2y + K * derivative1y;  

    return force;
}

Force total_force(Force drone, Force obstacle, Force target) {
    Force total;
    total.x = drone.x + obstacle.x + target.x;
    total.y = drone.y + obstacle.y + target.y;
    
    return total;
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
    fillDirections(directions, &directionCount);        // Simulate user input
 

    Obstacles obstacles = createObstacles(drone);       // Simulate communication with obstacles
    
    Targets targets = createTargets(drone);             // Simulate communication with targets


    //for (int i = 0; i < MAX_DIRECTIONS; i++) printf("%s\n", directions[i]);

    float K = 1.0;
    Force force_d, force_o, force_t;
    Force force;


    while (1) {
        targetsMoving(targets);
        obstaclesMoving(obstacles);


        force_d = drone_force(&drone, DRONEMASS, K);
        force_o = obstacle_force(&drone, obstacles);
        force_t = target_force(&drone, targets);
        force = total_force(force_d, force_o, force_t);

        
        updatePosition(&drone, directions[0], force);
        if (drone.x < 0 || drone.y < 0 || drone.x > WINDOW_LENGTH || drone.y > WINDOW_WIDTH) printf("ERROR");

        //printf("Removing direction: %s\n", directions[0]);
        removeFirstElement(directions, &directionCount);


        usleep(1000000/PERIOD); 
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
