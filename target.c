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
#define PERIODT 100000

int pid;
int fds[4]; 
FILE *file;


Message status;
Message msg;

char drone_str[80];
char str[len_str_targets];

int canSpawnPrev(int x_pos, int y_pos, Targets targets) {
    for (int i = 0; i < numTarget + status.level; i++) {
        if (abs(x_pos - targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - targets.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

Targets createTargets(Drone_bb drone) {
    Targets targets;
    int x_pos, y_pos;

    //Targets init
    for( int i = 0; i < MAX_TARGET; i++){
        targets.x[i] = 0;
        targets.y[i] = 0;
        targets.value[i] = 0;
    }

    for (int i = 0; i < numTarget + status.level; i++)
    {
        do{
            x_pos = rand() % (WINDOW_LENGTH-1);
            y_pos = rand() % (WINDOW_WIDTH-1);
        } while (
            (abs(x_pos - drone.x )<= NO_SPAWN_DIST) &&
            (abs(y_pos - drone.y) <= NO_SPAWN_DIST) || 
            canSpawnPrev(x_pos, y_pos, targets) == 0);

        targets.x[i] = x_pos;
        targets.y[i] = y_pos;
        targets.value[i] = i+1;
    }
    return targets;
}

void targetsMoving(Targets targets) {
    int num_moves = sizeof(moves) / sizeof(moves[0]);
    for (int i = 0; i < numTarget + status.level; i++) {
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

void refreshMap(){

    status.msg = 'R';

    // send drone position to target
    writeMsg(fds[askwr], &status, 
            "[TARGET] Ready not sended correctly", file);

    status.msg = '\0';

    readMsg(fds[recrd], &msg, &status, 
            "[TARGET] Error reading drone position from [BB]", file);

    status.targets = createTargets(status.drone);             // Create target vector

    writeMsg(fds[askwr], &status, 
            "[TARGET] Error sending target position to [BB]", file);
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(TARGET, file);
    }else if(signo == SIGTERM){
        fprintf(file, "Target is quitting\n");
        fflush(file);   
        fclose(file);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
   
    fdsRead(argc, argv, fds);
    
    // Opening log file
    file = fopen("outputtarget.txt", "a");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    pid = writePid("log.txt", 'a', 1, 't');

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    signal(SIGUSR1, sig_handler);
    signal(SIGTERM,sig_handler);

    fprintf(file, "Ready to read the drone position\n");
    fflush(file);

    readMsg(fds[recrd], &msg, &status, 
            "[TARGET] Error reading drone position from [BB]", file);

    fprintf(file, "Received drone position: %d,%d\n", status.drone.x, status.drone.y);
    fflush(file);

    status.targets = createTargets(status.drone);             // Create target vector

    for(int i = 0; i < MAX_TARGET; i++ ){
        fprintf(file, "targ[%d] = %d,%d,%d\n", i, status.targets.x[i], status.targets.y[i], status.targets.value[i]);
        fflush(file);
    }

    writeMsg(fds[askwr], &status, 
            "[TARGET] Error sending target position to [BB]", file);
    
    fprintf(file,"New targets sent");
    fflush(file);

    while (1) {

        refreshMap();
        usleep(PERIODT);
    }
}
