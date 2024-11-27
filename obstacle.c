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

void sig_handler(int signo) {
    if (signo == SIGUSR1)
    {
        handler(OBSTACLE, 100);
    }
}

int canSpawn(int x_pos, int y_pos, Targets targets) {
    for (int i = 0; i < NUM_TARGET; i++) {
        if (x_pos - targets.x[i] <= NO_SPAWN_DIST && y_pos - targets.y[i] <= NO_SPAWN_DIST) return 0; 
    }
    return 1;
}

Obstacles createObstacles(Drone_bb drone, Targets targets) {
    Obstacles obstacles;
    float x_pos, y_pos;

    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        do
        {
            x_pos = rand() % (WINDOW_LENGTH-4);
            y_pos = rand() % (WINDOW_WIDTH-4);
        } while (
            (x_pos >= drone.x - NO_SPAWN_DIST && x_pos <= drone.x + NO_SPAWN_DIST) &&
            (y_pos >= drone.y - NO_SPAWN_DIST && y_pos <= drone.y + NO_SPAWN_DIST) && 
            canSpawn(x_pos, y_pos, targets) == 1);

        obstacles.x[i] = x_pos;
        obstacles.y[i] = y_pos;
    }
    return obstacles;
}
// Simulate obstacle moving

void obstaclesMoving(Obstacles obstacles)
{
    int num_moves = sizeof(moves) / sizeof(moves[0]);
    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        const char *move = moves[rand() % num_moves];

        int up_condition = (obstacles.y[i] > 1);
        int down_condition = (obstacles.y[i] < WINDOW_WIDTH - 1);
        int right_condition = (obstacles.x[i] < WINDOW_LENGTH - 1);
        int left_condition = (obstacles.x[i] > 1);

        if (strcmp(move, "up") == 0 && up_condition)
            obstacles.y[i] -= 1;
        else if (strcmp(move, "down") == 0 && down_condition)
            obstacles.y[i] += 1;
        else if (strcmp(move, "right") == 0 && right_condition)
            obstacles.x[i] += 1;
        else if (strcmp(move, "left") == 0 && left_condition)
            obstacles.x[i] -= 1;

        else if (strcmp(move, "upleft") == 0 && up_condition && left_condition)
        {
            obstacles.x[i] -= (float)1.0 / sqrt(2);
            obstacles.y[i] -= (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "upright") == 0 && up_condition && right_condition)
        {
            obstacles.x[i] += (float)1.0 / sqrt(2);
            obstacles.y[i] -= (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "downleft") == 0 && down_condition && left_condition)
        {
            obstacles.x[i] -= (float)1.0 / sqrt(2);
            obstacles.y[i] += (float)1.0 / sqrt(2);
        }
        else if (strcmp(move, "downright") == 0 && down_condition && right_condition)
        {
            obstacles.x[i] += (float)1.0 / sqrt(2);
            obstacles.y[i] += (float)1.0 / sqrt(2);
        }
    }
}


int main(int argc, char *argv[])
{
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

    pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "o%d,", pid);

    if (writeSecure("log.txt", dataWrite, 1, 'a') == -1) {
        perror("Error in writing in log.txt");
        exit(1);
    }
    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    Drone_bb drone;
    Targets targets;
    Obstacles obstacles;

    char drone_str[10];
    char targets_str[len_str_targets];
    char obstacle_str[len_str_obstacles];
    char dronetarget_str[10 + len_str_targets];


    while (1) {

        // Read drone position
        if (read(fds[recrd], &dronetarget_str, sizeof(dronetarget_str)) == -1){
            perror("[OB] Error reading drone and target position from [BB]");
            exit(EXIT_FAILURE);
        }

        fprintf(file, "Reading drone and target position: %s\n", dronetarget_str);
        fflush(file);

        fromStringtoPositions(&drone, targets.x, targets.y, dronetarget_str, file);
        obstacles = createObstacles(drone, targets);

        // Sending obstacles positions
        fromPositiontoString(obstacles.x, obstacles.y, NUM_OBSTACLES, obstacle_str, sizeof(obstacle_str), file);

        fprintf(file, "Sending obstacle positions: %s\n", obstacle_str);
        fflush(file);
        if (write(fds[askwr], &obstacle_str, sizeof(obstacle_str)) == -1) {
            perror("[TA] Error sending obstacle positions to [BB]");
            exit(EXIT_FAILURE);
        }
        usleep(100000);
        // obstaclesMoving(obstacles);
    }

    // Close file
    fclose(file);
    return 0;
}
