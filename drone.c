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


// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define MAX_DIRECTIONS 80
#define WINDOW_WIDTH 400
#define WINDOW_LENGHT 800

typedef struct {
    float x;
    float y;

} Element;

typedef struct {
    int x;
    int y;
} Element_bb;

const char* moves[] = {"up", "down", "right", "left", "upleft", "upright", "downleft", "downright"};
Element drone = {10.0, 20.0};  

// Useful for debugging
void printPosition(Element p) {
    printf("Position: (x = %f, y = %f)\n", p.x, p.y);
    printf("BB Pos: (x = %.0f, y = %.0f)\n", round(p.x), round(p.y));
}

// Simulate user input
void fillDirections(const char* filled[], int* size) {
    for (int i = 0; i < *size; i++) {
        int randomIndex = rand() % 8;
        filled[i] = moves[randomIndex]; 
    }
}

// Update drone position
void updatePosition(Element* p, const char* direction) {
    int up_condition = (p->y > 1) ? 1 : 0;
    int down_condition = (p->y < WINDOW_WIDTH - 1) ? 1 : 0;
    int right_condition = (p->x < WINDOW_LENGHT - 1) ? 1 : 0;
    int left_condition = (p->x > 1) ? 1 : 0;


    if (strcmp(direction, "up") == 0 && up_condition) p->y -= 1;
    else if (strcmp(direction, "down") == 0 && down_condition) p->y += 1;
    else if (strcmp(direction, "right") == 0 && right_condition) p->x += 1;
    else if (strcmp(direction, "left") == 0 && left_condition) p->x -= 1;

    else if (strcmp(direction, "upleft") == 0 && up_condition && left_condition) {
        // printf("Before -");
        // printPosition(drone);
        p->x -= 1.0 / sqrt(2);
        p->y -= 1.0 / sqrt(2);
        // printf("Upleft - ");
        // printPosition(drone);
        }
    else if (strcmp(direction, "upright") == 0 && up_condition && right_condition) {
        p->x += 1.0 / sqrt(2);
        p->y -= 1.0 / sqrt(2);
    }
    else if (strcmp(direction, "downleft") == 0 && down_condition && left_condition) {
        p->x -= 1.0 / sqrt(2);
        p->y += 1.0 / sqrt(2);
    }
    else if (strcmp(direction, "downright") == 0 && down_condition && right_condition) {
        p->x += 1.0 / sqrt(2);
        p->y += 1.0 / sqrt(2);
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

    // Simulate user input
    fillDirections(directions, &directionCount);
    //for (int i = 0; i < MAX_DIRECTIONS; i++) printf("%s\n", directions[i]);


    while (1) {
        updatePosition(&drone, directions[0]);
        if (drone.x < 0 || drone.y < 0 || drone.x > WINDOW_LENGHT || drone.y > WINDOW_WIDTH) printf("ERROR");

        //printf("Removing direction: %s\n", directions[0]);
        removeFirstElement(directions, &directionCount);


        usleep(100000); 
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
