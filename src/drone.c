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
#include "log.h"

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define PERIOD 10 //50 //[Hz]
#define DRONEMASS 1

#define MAX_DIRECTIONS 80

float K = 1.0;

Force force_d = {0, 0};
Force force_o = {0, 0};
Force force_t = {0, 0};

Force force = {0, 0};

Speed speedPrev = {0, 0};
Speed speed = {0, 0};

Targets targets;
Obstacles obstacles;
Message status;
Message msg;

int pid;
int fds[4];

FILE *droneFile = NULL;

typedef struct
{
    float x;
    float y;
    float previous_x[2]; // 0 is one before and 1 is is two before
    float previous_y[2];

} Drone;



void updatePosition(Drone *p, Force force, int mass, Speed *speed, Speed *speedPrev, FILE* droneFile) {

    float x_pos = (2*mass*p->previous_x[0] + PERIOD*K*p->previous_x[0] + force.x*PERIOD*PERIOD - mass * p->previous_x[1]) / (mass + PERIOD * K);
    float y_pos = (2*mass*p->previous_y[0] + PERIOD*K*p->previous_y[0] + force.y*PERIOD*PERIOD - mass * p->previous_y[1]) / (mass + PERIOD * K);

    p->x = x_pos;
    p->y = y_pos;

    // LOGPOSITION(p);

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
    // To avoid having the drone still trying to move where it's not supposed to
    if (p->x > WINDOW_WIDTH || p->x < 1) force_d.x = 0;
    if (p->y > WINDOW_LENGTH || p->y < 1) force_d.y = 0;

}

void obstacle_force(Drone *drone, Obstacles* obstacles, FILE* droneFile) {
    // Force force = {0, 0};
    float deltaX, deltaY, distance, distance2, alpha, adjustedForceX, adjustedForceY;
    force_o.x = 0;
    force_o.y = 0;

    for (int i = 0; i < numObstacle + status.level; i++) {
        deltaX = drone->x - obstacles->x[i];
        deltaY = drone->y - obstacles->y[i];
        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));
        // fprintf(droneFile, "%f\t ", distance);
        // fflush(droneFile);
        if (distance > FORCE_THRESHOLD) {
            continue; // Beyond influence radius
        }
        // fprintf(droneFile,"(added %d)\t", i);
        // fflush(droneFile);
        float repulsion =ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
        if (repulsion > MAX_FORCE) repulsion = MAX_FORCE;
        adjustedForceX = repulsion * cos(alpha);
        adjustedForceY = repulsion * sin(alpha);

        force_o.x -= adjustedForceX;
        force_o.y -= adjustedForceY;
    }

}

void target_force(Drone *drone, Targets* targets, FILE* droneFile) {
    
    float deltaX, deltaY, distance, distance2;
    force_t.x = 0;
    force_t.y = 0;

    for (int i = 0; i < numTarget + status.level; i++) {
        if(targets->value[i] > 0){    
            deltaX = targets->x[i] - drone->x;
            deltaY = targets->y[i] - drone->y;
            distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

            fprintf(droneFile, "%f\t ", distance);
            fflush(droneFile);

            if (distance > FORCE_THRESHOLD)
                continue;
            if (distance < FORCE_THRESHOLD/2) continue;

            // fprintf(droneFile, "(added)\t");
            // fflush(droneFile);

            float attraction = ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
            force_t.x += attraction * (deltaX / distance);
            force_t.y += attraction * (deltaY / distance);
        }
    }

}

Force total_force(Force drone, Force obstacle, Force target, FILE* droneFile){
    Force total;
    total.x = drone.x + obstacle.x + target.x;
    total.y = drone.y + obstacle.y + target.y;

    // LOGFORCES(drone, target, obstacle);

    return total;
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(DRONE);
    }else if(signo == SIGTERM){
        // fprintf(droneFile, "Drone is quitting\n");
        // fflush(droneFile);   
        // LOGPROCESSDIED();
        fclose(droneFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

void newDrone (Drone* drone, Targets* targets, Obstacles* obstacles, char* directions, FILE* droneFile, char inst){
    target_force(drone, targets, droneFile);
    obstacle_force(drone, obstacles, droneFile);
    if(inst == 'I'){
        drone_force(drone, DRONEMASS, K, directions);
    }
    force = total_force(force_d, force_o, force_t, droneFile);

    updatePosition(drone, force, DRONEMASS, &speed,&speedPrev, droneFile);
}

void droneUpdate(Drone* drone, Speed* speed, Force* force, Message* msg) {

    msg->drone.x = (int)round(drone->x);
    msg->drone.y = (int)round(drone->y);
    msg->drone.speedX = speed->x;
    msg->drone.speedY = speed->y;
    msg->drone.forceX = force->x;
    msg->drone.forceY = force->y;
}

void mapInit(Drone* drone, Message* status, Message* msg){

    
    msgInit(status);

    fprintf(droneFile, "Updating drone position\n");
    fflush(droneFile);

    droneUpdate(drone, &speed, &force, status);

    //printMessageToFile(droneFile, status);
    // LOGDRONEINFO(status->drone);


    writeMsg(fds[askwr], status, 
            "[DRONE] Error sending drone info", droneFile);
    
    fprintf(droneFile, "Sent drone position\n");
    fflush(droneFile);
    
    readMsg(fds[recrd], status,
            "[DRONE] Error receiving map from BB", droneFile);
}

int main(int argc, char *argv[]) {
    
    fdsRead(argc, argv, fds);

    // Opening log file
    droneFile = fopen("log/outputdrone.log", "a");
    if (droneFile == NULL) {
        perror("[DRONE] Error during the file opening");
        exit(EXIT_FAILURE);
    }

    pid = writePid("log/log.txt", 'a', 1, 'd');

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    signal(SIGUSR1, sig_handler);
    signal(SIGTERM, sig_handler);
    
    char directions[MAX_DIRECTIONS] = {0};

    Drone drone = {0};

    drone.x = 10;
    drone.y = 20;
    drone.previous_x[0] = 10.0;
    drone.previous_x[1] = 10.0;
    drone.previous_y[0] = 20.0;
    drone.previous_y[1] = 20.0;

    for (int i = 0; i < MAX_TARGET; i++) {
        targets.x[i] = 0;
        targets.y[i] = 0;
        status.targets.x[i] = 0;
        status.targets.y[i] = 0;
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles.x[i] = 0;
        obstacles.y[i] = 0;
        status.obstacles.x[i] = 0;
        status.obstacles.y[i] = 0;
    }

    char data[200];

   mapInit(&drone, &status, &msg);
   // LOGNEWMAP(status);

    while (1)
    {
        status.msg = 'R';

        fprintf(droneFile, "Sending ready msg");
        fflush(droneFile);

        writeMsg(fds[askwr], &status, 
            "[DRONE] Ready not sended correctly", droneFile);

        status.msg = '\0';

        readMsg(fds[recrd], &status,
            "[DRONE] Error receiving map from BB", droneFile);

        switch (status.msg) {
        
            case 'M':

                newDrone(&drone, &status.targets, &status.obstacles, directions,droneFile,status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                // drone sends its position to BB
                writeMsg(fds[askwr], &status, 
                        "[DRONE-M] Error sending drone position", droneFile);
                break;
            case 'I':

                strcpy(directions, status.input);

                newDrone(&drone, &status.targets, &status.obstacles, directions,droneFile,status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                // drone sends its position to BB
                writeMsg(fds[askwr], &status, 
                        "[DRONE-I] Error sending drone position", droneFile);

                break;
            case 'A':
                
                newDrone(&drone, &status.targets, &status.obstacles, directions,droneFile,status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                fprintf(droneFile, "Drone updated position: %d,%d\n", status.drone.x, status.drone.y);
                fflush(droneFile);
                
                // drone sends its position to BB
                writeMsg(fds[askwr], &status, 
                        "[DRONE-A] Error sending drone position", droneFile);
                usleep(10000);
                break;
            default:
                perror("[DRONE-DEFAULT] Error data received");
                exit(EXIT_FAILURE);
        }

        usleep(1000000 / PERIOD);
    }
}
