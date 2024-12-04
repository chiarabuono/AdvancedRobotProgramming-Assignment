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

float K = 1.0;

int pid;

typedef struct
{
    float x;
    float y;
    float previous_x[2]; // 0 is one before and 1 is is two before
    float previous_y[2];

} Drone;

void setPosition(){
    ;
}
// Useful for debugging
void printPosition(Drone p) {
    printf("Position: (x = %f, y = %f)\n", p.x, p.y);
    printf("BB Pos: (x = %.0f, y = %.0f)\n", round(p.x), round(p.y));
}

// Simulate user input
void fillDirections(const char *filled[], int *size) {
    for (int i = 0; i < *size; i++) {
        int randomIndex = 7;
        filled[i] = moves[randomIndex];
    }
}

// Update drone position
Drone updatePosition(Drone *p, char *direction, Force force) {

    p->previous_x[1] = p->previous_x[0];  // Il secondo valore precedente diventa il primo
    p->previous_x[0] = p->x;  // Aggiorna il primo valore precedente con la posizione attuale
    p->previous_y[1] = p->previous_y[0];  // Lo stesso per y
    p->previous_y[0] = p->y;

    // conditions to stay inside the window
    // int up_condition = (p->y > 0) ? 1 : 0;
    // int down_condition = (p->y < WINDOW_WIDTH - 1) ? 1 : 0;
    // int right_condition = (p->x < WINDOW_LENGTH - 1) ? 1 : 0;
    // int left_condition = (p->x > 0) ? 1 : 0;

    p->y += force.y;
    p->x += force.x;

    if (p->x < 0) p->x = 0;
    if (p->y < 0) p->y = 0;
    if (p->x >= WINDOW_LENGTH) p->x = WINDOW_LENGTH - 1;
    if (p->y >= WINDOW_WIDTH) p->y = WINDOW_WIDTH - 1;
    return *p;
}


Drone_bb DroneToDrone_bb(Drone *drone) {
    Drone_bb bb = {(int)round(drone->x), (int)round(drone->y)};
    return bb;
}

Force drone_force(Drone *p, float mass, float K, char* direction) {
    Force force = {0, 0};

    // Calcolo delle derivate del movimento
    float derivative1x = (p->x - p->previous_x[0]) / PERIOD;
    float derivative2x = (p->previous_x[1] + p->x - 2 * p->previous_x[0]) / (PERIOD * PERIOD);
    float derivative1y = (p->y - p->previous_y[0]) / PERIOD;
    float derivative2y = (p->previous_y[1] + p->y - 2 * p->previous_y[0]) / (PERIOD * PERIOD);

    // Forze derivate dalle equazioni del moto
    force.x = mass * derivative2x + K * derivative1x + 1;
    force.y = mass * derivative2y + K * derivative1y + 1;

    // Verifica della validità di direction
    
    if (strcmp(direction, "") != 0) {
        // Imposta direzione x
        if (strcmp(direction, "right") == 0 || strcmp(direction, "upright") == 0 || strcmp(direction, "downright") == 0) {
            force.x = fabs(force.x);
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "downleft") == 0) {
            force.x = -fabs(force.x);
        } else if (strcmp(direction, "up") == 0 || strcmp(direction, "down") == 0 || strcmp(direction, "center") == 0) {
            force.x = 0; // Nessuna forza lungo x
        } 

        // Imposta direzione y
        if (strcmp(direction, "up") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "upright") == 0) {
            force.y = -fabs(force.y);
        } else if (strcmp(direction, "down") == 0 || strcmp(direction, "downleft") == 0 || strcmp(direction, "downright") == 0) {
            force.y = fabs(force.y);
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "right") == 0 || strcmp(direction, "center") == 0 ) {
            force.y = 0; // Nessuna forza lungo y
        }
    } else {
        force.x = 0;
        force.y = 0;
    }

    return force;
}

// Force drone_force(Drone *p, float mass, float K, char* direction) {

//     Force force;

//     float derivative1x = (p->x - p->previous_x[0]) / PERIOD;
//     float derivative2x = (p->previous_x[1] + p->x - 2 * p->previous_x[0]) / (PERIOD * PERIOD);
//     force.x = mass * derivative2x + K * derivative1x;
//     if (direction != NULL) force.x += 1;

//     float derivative1y = (p->y - p->previous_y[0]) / PERIOD;
//     float derivative2y = (p->previous_y[1] + p->y - 2 * p->previous_y[0]) / (PERIOD * PERIOD);
//     force.y = mass * derivative2y + K * derivative1y;
//     if (direction != NULL) force.y = 1;

//     if (strcmp(direction, "up") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "upright") == 0)
//         force.y = -abs(force.y);
//     if (strcmp(direction, "right") == 0 || strcmp(direction, "upright") == 0 || strcmp(direction, "downright") == 0) 
//         force.x =  abs(force.x);
//     if (strcmp(direction, "down") == 0 || strcmp(direction, "downleft") == 0 || strcmp(direction, "downright") == 0)  
//         force.y =  abs(force.y);
//     if (strcmp(direction, "left") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "downleft") == 0)  
//         force.x = -abs(force.x);    

//     if (strcmp(direction, "up") == 0 || strcmp(direction, "down") == 0) {
//         force.x = 0;
//     }
//     else if (strcmp(direction, "left") == 0 || strcmp(direction, "right") == 0) {
//         force.y = 0;
//     }
    

//     return force;
// }



Force obstacle_force(Drone *drone, Obstacles obstacles, FILE* file) {
    Force force = {0, 0};
    float deltaX, deltaY, distance, distance2, alpha, adjustedForceX, adjustedForceY;

    for (int i = 0; i < numObstacle; i++) {
        deltaX = drone->x - obstacles.x[i];
        deltaY = drone->y - obstacles.y[i];
        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

        if (distance > FORCE_THRESHOLD) {
            continue; // Beyond influence radius
        }

        float repulsion = 1000*ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
        adjustedForceX = repulsion * cos(alpha);
        adjustedForceY = repulsion * sin(alpha);

        force.x -= adjustedForceX;
        force.y -= adjustedForceY;
    }

    return force;
}


Force target_force(Drone *drone, Targets targets) {
    Force force = {0, 0};
    float deltaX, deltaY, distance, distance2;

    for (int i = 0; i < numTarget; i++) {
        deltaX = targets.x[i] - drone->x;
        deltaY = targets.y[i] - drone->y;
        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

        if (distance > FORCE_THRESHOLD)
            continue;

        float attraction = 10*ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
        force.x += attraction * (deltaX / distance);
        force.y += attraction * (deltaY / distance);
    }

    return force;
}

Force total_force(Force drone, Force obstacle, Force target, FILE* file){
    Force total;
    total.x = drone.x + obstacle.x + target.x;
    total.y = drone.y + obstacle.y + target.y;

    return total;
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(DRONE, 100);
    }
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

    Drone drone = {10.0, 20.0, {10.0, 20.0}, {10.0, 20.0}};
    Drone_bb drone_bb;
    Force force_d = {0, 0};
    Force force_o = {0, 0};
    Force force_t = {0, 0};
    Force force;
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
    int bytesRead;

    drone_bb = DroneToDrone_bb(&drone);
    
    if (drone_bb.x < 10 && drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "0%d;0%d", drone_bb.x, drone_bb.y);
    else if (drone_bb.x < 10) snprintf(drone_str, sizeof(drone_str), "0%d;%d", drone_bb.x, drone_bb.y);
    else if (drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "%d;0%d", drone_bb.x, drone_bb.y);
    else snprintf(drone_str, sizeof(drone_str), "%d;%d", drone_bb.x, drone_bb.y);

    if (write(fds[askwr], drone_str, sizeof(drone_str)) == -1) {
        perror("[DRONE] Error sending drone position");
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
            
    //sleep(1);

    while (1)
    {
        if (write(fds[askwr], "R", 2) == -1) {
                perror("[DRONE] Ready not sended correctly\n");
                exit(EXIT_FAILURE);
            }
        fprintf(file, "[DRONE] Drone ready\n");
        fflush(file);

        bytesRead = read(fds[recrd], &data, sizeof(data));
        if (bytesRead == -1) {
            perror("[DRONE] Error receiving data from BB");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "(%d, %d)\n", (int)drone.x, (int)drone.y);
        fflush(file);
        switch (data[0]) {
        
        case 'M':
            memmove(data, data + 1, strlen(data));
            fprintf(file, "[M] Received new map: %s\n", data);
            fromStringtoPositionsWithTwoTargets(targets.x, targets.y, obstacles.x, obstacles.y, data, file);
            // fprintf(file, "[M] Read new map\n");
            // fflush(file);

            // computing new force on the drone
            force_t = target_force(&drone, targets);
            force_o = obstacle_force(&drone, obstacles, file);
            force_d = drone_force(&drone, DRONEMASS, K, directions);
            force = total_force(force_d, force_o, force_t, file);

            // 'A' case

            // fprintf(file, "[M] Updating drone position\n");
            // fflush(file);
            drone = updatePosition(&drone, directions, force);
            drone_bb = DroneToDrone_bb(&drone);
            

            if (drone_bb.x < 10 && drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "0%d;0%d", drone_bb.x, drone_bb.y);
            else if (drone_bb.x < 10) snprintf(drone_str, sizeof(drone_str), "0%d;%d", drone_bb.x, drone_bb.y);
            else if (drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "%d;0%d", drone_bb.x, drone_bb.y);
            else snprintf(drone_str, sizeof(drone_str), "%d;%d", drone_bb.x, drone_bb.y);

            // drone sends its position to BB
            if (write(fds[askwr], drone_str, strlen(drone_str)) == -1) {
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
            force_t = target_force(&drone, targets);
            force_o = obstacle_force(&drone, obstacles, file);
            force_d = drone_force(&drone, DRONEMASS, K, directions);
            force = total_force(force_d, force_o, force_t, file);

            drone = updatePosition(&drone, directions, force);
            drone_bb = DroneToDrone_bb(&drone);
            

            if (drone_bb.x < 10 && drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "0%d;0%d", drone_bb.x, drone_bb.y);
            else if (drone_bb.x < 10) snprintf(drone_str, sizeof(drone_str), "0%d;%d", drone_bb.x, drone_bb.y);
            else if (drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "%d;0%d", drone_bb.x, drone_bb.y);
            else snprintf(drone_str, sizeof(drone_str), "%d;%d", drone_bb.x, drone_bb.y);

            // drone sends its position to BB
            if (write(fds[askwr], drone_str, strlen(drone_str)) == -1) {
                perror("[I] Error sending drone position");
                exit(EXIT_FAILURE);
            }

            break;
        case 'A':
            force_t = target_force(&drone, targets);
            force_o = obstacle_force(&drone, obstacles, file);
            force_d = drone_force(&drone, DRONEMASS, K, directions);
            force = total_force(force_d, force_o, force_t, file);

            drone = updatePosition(&drone, directions, force);
            drone_bb = DroneToDrone_bb(&drone);
            
            if (drone_bb.x < 10 && drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "0%d;0%d", drone_bb.x, drone_bb.y);
            else if (drone_bb.x < 10) snprintf(drone_str, sizeof(drone_str), "0%d;%d", drone_bb.x, drone_bb.y);
            else if (drone_bb.y < 10) snprintf(drone_str, sizeof(drone_str), "%d;0%d", drone_bb.x, drone_bb.y);
            else snprintf(drone_str, sizeof(drone_str), "%d;%d", drone_bb.x, drone_bb.y);

            // drone sends its position to BB
            if (write(fds[askwr], drone_str, strlen(drone_str)) == -1) {
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
