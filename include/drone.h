#ifndef DRONE_H
#define DRONE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
extern FILE *droneFile;


#define LOGNEWMAP(status) {                                                      \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(droneFile, "%s New map created.\n", date);                             \
    fprintf(droneFile, "\tTarget positions: ");                                      \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        if (status.targets.x[t] == 0 && status.targets.y[t] == 0) break;         \
        fprintf(droneFile, "(%d, %d) [val: %d] ",                                  \
                status.targets.x[t], status.targets.y[t], status.targets.value[t]); \
    }                                                                            \
    fprintf(droneFile, "\n\tObstacle positions: ");                                  \
    for (int t = 0; t < MAX_OBSTACLES; t++) {                                    \
        if (status.obstacles.x[t] == 0 && status.obstacles.y[t] == 0) break;     \
        fprintf(droneFile, "(%d, %d) ",                                            \
                status.obstacles.x[t], status.obstacles.y[t]);                   \
    }                                                                            \
    fprintf(droneFile, "\n");                                                      \
    fflush(droneFile);                                                             \
}


#define LOGPROCESSDIED() { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                         \
    fprintf(droneFile, "%s Process dead\n", date);                              \
    fflush(droneFile);                                                             \
}

#if USE_DEBUG
#define LOGPOSITION(drone) { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Drone info - ", date); \
    fprintf(droneFile, "Pre-previous position (%d, %d) ", drone->previous_x[1], drone->previous_y[1]); \
    fprintf(droneFile, "Previous position (%d, %d) ", drone.previous_x[0], drone.previous_y[0]); \
    fprintf(droneFile, "Actual position (%d, %d) ", drone.x, drone.y); \
    fflush(droneFile); \
}

#else
#define LOGPOSITION(drone) {
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Drone position (%d, %d) ", date, drone.x, drone.y); \
    fflush(droneFile); \
}
#endif

#define LOGDRONEINFO(droneInfo) { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Drone info - ", date); \
    fprintf(droneFile, "Position (%d, %d) ", droneInfo.x, droneInfo.y); \
    fprintf(droneFile, "Speed (%.2f, %.2f) ", droneInfo.speedX, droneInfo.speedY); \
    fprintf(droneFile, "Force (%.2f, %.2f) ", droneInfo.forceX, droneInfo.forceY); \
    fprintf(droneFile, "\n"); \
    fflush(droneFile); \
}

#if USE_DEBUG
#define LOGFORCES(force_d, force_t, force_o) { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Forces on the drone - ", date); \
    fprintf(droneFile, "Drone force (%.2f, %.2f) ", force_d.x, force_d.y); \
    fprintf(droneFile, "Target force (%.2f, %.2f) ", force_t.x, force_t.y); \
    fprintf(droneFile, "Obstacle force (%.2f, %.2f)\n", force_o.x, force_o.y); \
    fflush(droneFile); \
}
#else
#define LOGFORCES(force_d, force_t, force_o) {}
#endif




#endif // LOG_H
