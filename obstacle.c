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

typedef struct
{
    float x[NUM_OBSTACLES];
    float y[NUM_OBSTACLES];
} Obstacles;

#define OBSTACLE_MASS 0.1

int pid;

void sig_handler(int signo)
{
    if (signo == SIGUSR1)
    {
        handler(OBSTACLE, 100);
    }
}

Obstacles createObstacles(Drone_bb drone)
{
    Obstacles obstacles;
    float x_pos, y_pos;

    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        do
        {
            x_pos = rand() % WINDOW_LENGTH;
            y_pos = rand() % WINDOW_WIDTH;
        } while (
            (x_pos >= drone.x - NO_SPAWN_DIST && x_pos <= drone.x + NO_SPAWN_DIST) &&
            (y_pos >= drone.y - NO_SPAWN_DIST && y_pos <= drone.y + NO_SPAWN_DIST));

        obstacles.x[i] = x_pos;
        obstacles.y[i] = y_pos;
    }
    return obstacles;
}

// Simulate obstacle moving
const char *moves[] = {"up", "down", "right", "left", "upleft", "upright", "downleft", "downright"};

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

Force obstacle_force(Drone_bb *drone, Obstacles obstacles)
{
    Force force = {0, 0};
    float deltaX, deltaY, distance, distance2;

    for (int i = 0; i < NUM_OBSTACLES; i++)
    {
        deltaX = drone->x - obstacles.x[i];
        deltaY = drone->y - obstacles.y[i];
        distance2 = pow(deltaX, 2) + pow(deltaY, 2);

        if (distance2 == 0 || distance2 > pow(FORCE_THRESHOLD, 2))
            continue; // Beyond influence radius

        distance = sqrt(distance2);

        float repulsion = ETA * pow((1 / distance - 1 / FORCE_THRESHOLD), 2) / distance;
        force.x += repulsion * (deltaX / distance); // Normalize direction
        force.y += repulsion * (deltaY / distance);
    }

    return force;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }

    // Opening log file
    FILE *file = fopen("outputobstacle.txt", "a");
    if (file == NULL)
    {
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
    while (token != NULL && index < 4)
    {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }

    pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "o%d,", pid);

    if (writeSecure("log.txt", dataWrite, 1, 'a') == -1)
    {
        perror("Error in writing in log.txt");
        exit(1);
    }
    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    Drone_bb drone;
    char drone_str[80];
    char force_str[80];

    Obstacles obstacles = createObstacles(drone); // Create obstacles vector
    Force force_o;


    while (1) {

        if (read(fds[recrd], &drone_str, sizeof(drone_str)) == -1){
            perror("[OB] Error reading drone position from [BB]");
            exit(EXIT_FAILURE);
        }
        fromStringtoDrone(&drone, drone_str, file);

        force_o = obstacle_force(&drone, obstacles);
        snprintf(force_str, sizeof(force_str), "%f,%f", force_o.x, force_o.y);

        if (write(fds[askwr], &force_str, sizeof(force_str)) == -1) {
            perror("[OB] Error sending force_o to [BB]");
            exit(EXIT_FAILURE);
        }

        obstaclesMoving(obstacles);
    }

    // Chiudiamo il file
    fclose(file);
    return 0;
}
