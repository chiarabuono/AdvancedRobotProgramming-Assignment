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
#include <signal.h>
#include <math.h>

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

// management target
#define MAX_TARGET_VALUE 9
#define PERIODT 100000

int pid;
float reset_period = 10; // [s]
float second = 1000000;
float reset = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(TARGET,100);
    }
}

Targets createTargets(Drone_bb drone) {
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


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    FILE *file = fopen("outputtarget.txt", "a");
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
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "t%d,", pid);

    if(writeSecure("log.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    Drone_bb drone;
    Targets targets;

    char drone_str[80];
    char str[len_str_targets];
    
    if (read(fds[recrd], &drone_str, sizeof(drone_str)) == -1){
        perror("[TA] Error reading drone position from [BB]");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Reading drone position and Computing target positio\n");
    fflush(file);

    fromStringtoDrone(&drone, drone_str, file);
    targets = createTargets(drone);             // Create target vector
    fromPositiontoString(targets.x, targets.y, NUM_TARGET, str, sizeof(str), file);

    fprintf(file, "Sending target position to [BB]\n");
    fflush(file);
    if (write(fds[askwr], &str, sizeof(str)) == -1) {
        perror("[TA] Error sending target position to [BB]");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Finish initialization\n");
    fflush(file);
    reset = 0;
    while (1) {
        reset += PERIODT/second;
        if (reset >= reset_period){
            reset = 0;
            if (write(fds[askwr], "R", 2) == -1) {
                    perror("[TARGET] Ready not sended correctly\n");
                    exit(EXIT_FAILURE);
                }
            fprintf(file, "[TARGET] target ready\n");
            fflush(file);

            if (read(fds[recrd], &drone_str, sizeof(drone_str)) == -1){
                perror("[TA] Error reading drone position from [BB]");
                exit(EXIT_FAILURE);
            }

            fprintf(file, "Reading drone position: %s\nComputing target position\n", drone_str);
            fflush(file);

            fromStringtoDrone(&drone, drone_str, file);
            targets = createTargets(drone);             // Create target vector
            fromPositiontoString(targets.x, targets.y, NUM_TARGET, str, sizeof(str), file);

            fprintf(file, "Sending target position to [BB]%s\n", str);
            fflush(file);
    
            if (write(fds[askwr], &str, sizeof(str)) == -1) {
                perror("[TA] Error sending target position to [BB]");
                exit(EXIT_FAILURE);
            }
        }
        // targetsMoving(targets);
        usleep(PERIODT);
    }
    
    // Close the file
    fclose(file);
    return 0;
}
