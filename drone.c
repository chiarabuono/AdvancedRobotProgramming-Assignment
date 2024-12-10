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
#include <signal.h>
// #include <errno.h>

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define MAX_DIRECTIONS 80
#define PERIOD 10 //50 //[Hz]
#define DRONEMASS 1

float K = 1;

Force force_d = {0, 0};
Force force_o = {0, 0};
Force force_t = {0, 0};

int pid;

typedef struct
{
    float x;
    float y;
    float previous_x[2]; // 0 is one before and 1 is is two before
    float previous_y[2];

} Drone;


// Simulate user input
void fillDirections(const char *filled[], int *size) {
    for (int i = 0; i < *size; i++) {
        int randomIndex = 7;
        filled[i] = moves[randomIndex];
    }
}

// Update drone position
void updatePosition(Drone *p, Force force, int mass, Speed *speed, Speed *speedPrev, FILE* file) {

    fprintf(file, "-----------------------------------------------\n");
    fprintf(file, "Prev. pos (%f, %f) ", p->x, p->y);
    float x_pos = (2*mass*p->previous_x[0] + PERIOD*K*p->previous_x[0] + force.x*PERIOD*PERIOD - mass * p->previous_x[1]) / (mass + PERIOD * K);
    float y_pos = (2*mass*p->previous_y[0] + PERIOD*K*p->previous_y[0] + force.y*PERIOD*PERIOD - mass * p->previous_y[1]) / (mass + PERIOD * K);

    p->x = x_pos;
    p->y = y_pos;

    fprintf(file, "Updated pos (%f, %f) \n", x_pos, y_pos);
    fprintf(file, "x1 used (%f, %f), x2 used (%f, %f) Force used (%f, %f)\n", p->previous_x[0], p->previous_y[0], p->previous_x[1], p->previous_y[1], force.x, force.y);


    if (p->x < 0) p->x = 0;
    if (p->y < 0) p->y = 0;
    if (p->x >= WINDOW_LENGTH) p->x = WINDOW_LENGTH - 1;
    if (p->y >= WINDOW_WIDTH) p->y = WINDOW_WIDTH - 1;

    p->previous_x[1] = p->previous_x[0]; 
    p->previous_x[0] = p->x;  
    p->previous_y[1] = p->previous_y[0];
    p->previous_y[0] = p->y;

    float speedX = (speedPrev->x + force.x/mass * (1.0f/PERIOD));
    float speedY = (speedPrev->y + force.y/mass * (1.0f/PERIOD));

    speedPrev->x = speed->x;
    speedPrev->y = speed->y;

    speed->x = speedX;
    speed->y = speedY;

    fflush(file);


}


Drone_bb DroneToDrone_bb(Drone *drone) {
    Drone_bb bb = {(int)round(drone->x), (int)round(drone->y)};
    return bb;
}

void drone_force(Drone *p, float mass, float K, char* direction) {
    // Force force = {0, 0};

    // Calcolo delle derivate del movimento
    // float derivative1x = (p->x - p->previous_x[0]) / PERIOD;
    // float derivative2x = (p->previous_x[1] + p->x - 2 * p->previous_x[0]) / (PERIOD * PERIOD);
    // float derivative1y = (p->y - p->previous_y[0]) / PERIOD;
    // float derivative2y = (p->previous_y[1] + p->y - 2 * p->previous_y[0]) / (PERIOD * PERIOD);

    // // Forze derivate dalle equazioni del moto
    // force.x = mass * derivative2x - K * derivative1x;
    // force.y = mass * derivative2y - K * derivative1y +1;

    // Verifica della validità di direction
    
    if (strcmp(direction, "") != 0) {
        // Imposta direzione x
        if (strcmp(direction, "right") == 0 || strcmp(direction, "upright") == 0 || strcmp(direction, "downright") == 0) {
            force_d.x += STEP;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "downleft") == 0) {
            force_d.x -= STEP;
        } else if (strcmp(direction, "up") == 0 || strcmp(direction, "down") == 0) {
            force_d.x += 0; // Nessuna forza lungo x
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.x = 0;
        }

        if (strcmp(direction, "up") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "upright") == 0) {
            force_d.y -= STEP;
        } else if (strcmp(direction, "down") == 0 || strcmp(direction, "downleft") == 0 || strcmp(direction, "downright") == 0) {
            force_d.y += STEP;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "right") == 0 ) {
            force_d.y += 0; // Nessuna forza lungo y
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.y = 0;
        }
    } else {
        force_d.x += 0;
        force_d.y += 0;
    }

    // if (force_d.x > STEP*10) force_d.x = STEP*10;
    // if (force_d.y > STEP*10) force_d.y = STEP*10;
    // if (force_d.x < -STEP*10) force_d.x = -STEP*10;
    // if (force_d.y < -STEP*10) force_d.y = -STEP*10;

}

void obstacle_force(Drone *drone, Obstacles obstacles, FILE* file) {
    // Force force = {0, 0};
    float deltaX, deltaY, distance, distance2, alpha, adjustedForceX, adjustedForceY;
    force_o.x = 0;
    force_o.y = 0;

    for (int i = 0; i < numObstacle; i++) {
        deltaX = drone->x - obstacles.x[i];
        deltaY = drone->y - obstacles.y[i];
        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));
        // fprintf(file, "%f\t ", distance);
        // fflush(file);
        if (distance > FORCE_THRESHOLD) {
            continue; // Beyond influence radius
        }
        fprintf(file,"(added %d)\t", i);
        fflush(file);
        float repulsion =ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
        adjustedForceX = repulsion * cos(alpha);
        adjustedForceY = repulsion * sin(alpha);

        force_o.x -= adjustedForceX;
        force_o.y -= adjustedForceY;
    }
    fprintf(file,"\n");
    fflush(file);

}


void target_force(Drone *drone, Targets targets, FILE* file) {
    // Force force = {0, 0};
    float deltaX, deltaY, distance, distance2;
    force_t.x = 0;
    force_t.y = 0;

    for (int i = 0; i < numTarget; i++) {
        deltaX = targets.x[i] - drone->x;
        deltaY = targets.y[i] - drone->y;
        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

        fprintf(file, "%f\t ", distance);
        fflush(file);

        if (distance > FORCE_THRESHOLD)
            continue;
        if (distance < FORCE_THRESHOLD/2) continue;

        fprintf(file, "(added)\t");
        fflush(file);

        float attraction = ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
        force_t.x += attraction * (deltaX / distance);
        force_t.y += attraction * (deltaY / distance);
    }
    fprintf(file, "\n");
    fflush(file);

}

Force total_force(Force drone, Force obstacle, Force target, FILE* file){
    Force total;
    total.x = drone.x + obstacle.x + target.x;
    total.y = drone.y + obstacle.y + target.y;

    fprintf(file, "FORCE: drone (%f, %f), obstacles (%f, %f), target (%f, %f)\n", drone.x, drone.y, obstacle.x, obstacle.y, target.x, target.y);
    fflush(file);

    return total;
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(DRONE, 100);
    }
}

void printDrone(int row, Drone drone, FILE* file) {
    fprintf(file, "[%d] Drone position: (%f, %f)\n", row, drone.x, drone.y);
    fflush(file);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }

    // Opening log file
    FILE *file = fopen("outputdrone.txt", "a");
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

    pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "d%d,", pid);

    if (writeSecure("log.txt", dataWrite, 1, 'a') == -1) {
        perror("Error in writing in log.txt");
        exit(1);
    }

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    // Simulate user input
    char directions[MAX_DIRECTIONS] = {0};

    Drone drone = {0};
    drone.x = 10;
    drone.y = 20;
    drone.previous_x[0] = 10.0;
    drone.previous_x[1] = 10.0;
    drone.previous_y[0] = 20.0;
    drone.previous_y[1] = 20.0;

    Drone_bb drone_bb;

    Force force = {0, 0};

    Speed speedPrev = {0, 0};
    Speed speed = {0, 0};

    Targets targets;
    Obstacles obstacles;

    for (int i = 0; i < MAX_TARGET; i++) {
        targets.x[i] = 0;
        targets.y[i] = 0;
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles.x[i] = 0;
        obstacles.y[i] = 0;
    }

    char data[200];
    char drone_str[6];
    char droneInfo_str[40];
    int bytesRead;

    drone_bb = DroneToDrone_bb(&drone);
    
    droneInfotoString(&drone_bb, &force, &speed, droneInfo_str, sizeof(droneInfo_str), file);
    fprintf(file, "[start] Drone info %s\n", droneInfo_str);
    fflush(file);
    if (write(fds[askwr], droneInfo_str, sizeof(droneInfo_str)) == -1) {
        perror("[DRONE] Error sending drone info");
        exit(EXIT_FAILURE);
    }

    if (read(fds[recrd], &data, sizeof(data)) == -1){
        perror("[DRONE] Error receiving map from BB");
        exit(EXIT_FAILURE);
    } 

    fprintf(file, "[DRONE] Received map: %s\n", data);
    fflush(file);
    memmove(data, data + 1, strlen(data));
    fromStringtoPositionsWithTwoTargets(targets.x, targets.y, obstacles.x, obstacles.y, data, file);
        
    while (1)
    {
        if (write(fds[askwr], "R", 2) == -1) {
                perror("[DRONE] Ready not sended correctly\n");
                exit(EXIT_FAILURE);
            }
        // fprintf(file, "[DRONE] Drone ready\n");
        // fflush(file);

        bytesRead = read(fds[recrd], &data, sizeof(data));
        if (bytesRead == -1) {
            perror("[DRONE] Error receiving data from BB");
            exit(EXIT_FAILURE);
        }
        switch (data[0]) {
        
        case 'M':
            memmove(data, data + 1, strlen(data));
            fprintf(file, "[M] Received new map: %s\n", data);
            fromStringtoPositionsWithTwoTargets(targets.x, targets.y, obstacles.x, obstacles.y, data, file);

            // computing new force on the drone
            target_force(&drone, targets, file);
            obstacle_force(&drone, obstacles, file);
            // drone_force(&drone, DRONEMASS, K, directions);
            force = total_force(force_d, force_o, force_t, file);

            // 'A' case
            // fprintf(file, "[M] Updating drone position\n");
            // fflush(file);
            updatePosition(&drone, force, DRONEMASS, &speed,&speedPrev, file);
            drone_bb = DroneToDrone_bb(&drone);
            

            droneInfotoString(&drone_bb, &force, &speed, droneInfo_str, sizeof(droneInfo_str), file);
            // fprintf(file, "[M] Drone info %s\n", droneInfo_str);
            // fflush(file);
            // drone sends its position to BB
            if (write(fds[askwr], droneInfo_str, strlen(droneInfo_str)) == -1) {
                perror("[M] Error sending drone position");
                exit(EXIT_FAILURE);
            }
            break;
        case 'I':
            char* extractedMsg = strchr(data, ';'); // Trova il primo ';'
                if (extractedMsg != NULL) {
                    extractedMsg++; // Salta il ';'
                } else {
                    extractedMsg = data; // Se ';' non trovato, usa l'intero messaggio
                }
            strcpy(directions, extractedMsg);

            // 'A' case
            target_force(&drone, targets, file);
            obstacle_force(&drone, obstacles, file);
            drone_force(&drone, DRONEMASS, K, directions);
            force = total_force(force_d, force_o, force_t, file);

            updatePosition(&drone, force, DRONEMASS, &speed,&speedPrev, file);
            drone_bb = DroneToDrone_bb(&drone);
            

            droneInfotoString(&drone_bb, &force, &speed, droneInfo_str, sizeof(droneInfo_str), file);
            // drone sends its position to BB
            if (write(fds[askwr], droneInfo_str, strlen(droneInfo_str)) == -1) {
                perror("[I] Error sending drone position");
                exit(EXIT_FAILURE);
            }

            break;
        case 'A':
            target_force(&drone, targets, file);
            obstacle_force(&drone, obstacles, file);
            // drone_force(&drone, DRONEMASS, K, directions);
            force = total_force(force_d, force_o, force_t, file);

            updatePosition(&drone, force, DRONEMASS, &speed,&speedPrev, file);
            drone_bb = DroneToDrone_bb(&drone);
            
            droneInfotoString(&drone_bb, &force, &speed, droneInfo_str, sizeof(droneInfo_str), file);

            // drone sends its position to BB
            if (write(fds[askwr], droneInfo_str, strlen(droneInfo_str)) == -1) {
                perror("[DRONE] Error sending drone position");
                exit(EXIT_FAILURE);
            }
            usleep(10000);
            break;
        default:
            perror("[DRONE] Error format string received");
            exit(EXIT_FAILURE);
        }

        usleep(1000000 / PERIOD);
    }

    // Chiudiamo il file
    fclose(file);
    return 0;
}