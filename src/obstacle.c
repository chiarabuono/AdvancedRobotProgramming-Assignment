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


#define OBSTACLE_MASS 0.1

int pid;
int fds[4];

Message status;
Message msg;

FILE *file;

void sig_handler(int signo) {
    if (signo == SIGUSR1)
    {
        handler(OBSTACLE);
    }else if(signo == SIGTERM){
        fprintf(file, "Obstacle is quitting\n");
        fflush(file);   
        fclose(file);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

int canSpawn(int x_pos, int y_pos, Targets targets) {
    for (int i = 0; i < numTarget + status.level; i++) {
        if (abs(x_pos - targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - targets.y[i]) <= NO_SPAWN_DIST) return 0; 
    }
    return 1;
}

int canSpawnPrev(int x_pos, int y_pos, Obstacles obstacles) {
    for (int i = 0; i < numTarget + status.level; i++) {
        if (abs(x_pos - obstacles.x[i]) <= NO_SPAWN_DIST && abs(y_pos - obstacles.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

Obstacles createObstacles(Drone_bb drone, Targets targets) {
    Obstacles obstacles;
    int x_pos, y_pos;

    for( int i = 0; i < MAX_OBSTACLES; i++){
        obstacles.x[i] = 0;
        obstacles.y[i] = 0;
    }
    
    for (int i = 0; i < numObstacle + status.level; i++){
    
        do {
            x_pos = rand() % (WINDOW_LENGTH - 1);
            y_pos = rand() % (WINDOW_WIDTH - 1);
        } while (
            ((abs(x_pos - drone.x) <= NO_SPAWN_DIST) &&
            (abs(y_pos - drone.y) <= NO_SPAWN_DIST)) || 
            canSpawn(x_pos, y_pos, targets) == 0 ||
            canSpawnPrev(x_pos, y_pos, obstacles) == 0);

        obstacles.x[i] = x_pos;
        obstacles.y[i] = y_pos;
    }

    return obstacles;
}

int main(int argc, char *argv[]) {
    
    fdsRead(argc, argv, fds);

    // Opening log file
    file = fopen("log/outputobstacle.txt", "a");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    pid = writePid("log/log.txt", 'a', 1, 'o');

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    signal(SIGUSR1, sig_handler);
    signal(SIGTERM, sig_handler);

    for( int i = 0; i < MAX_OBSTACLES; i++){
        status.obstacles.x[i] = 0;
        status.obstacles.y[i] = 0;
    }

    while (1) {

        // Read drone position
        readMsg(fds[recrd], &msg, &status, 
            "[OBSTACLE] Error reading drone and target position from [BB]", file);

        status.obstacles = createObstacles(status.drone, status.targets);

        fprintf(file, "Obstacle created:\n");
        for(int i = 0; i < MAX_TARGET; i++ ){
        fprintf(file, "obst[%d] = %d,%d\n", i, status.obstacles.x[i], status.obstacles.y[i]);
        fflush(file);
    }
        writeMsg(fds[askwr], &status, 
            "[OBSTACLE] Error sending obstacle positions to [BB]", file);

        usleep(100000);
    }
}
