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

#define PROCESSTOCONTROL 5

int pid[PROCESSTOCONTROL] = {0};  // Initialize PIDs to 0

int main(int argc, char *argv[]) {
    // Open the output file for writing
    FILE *file = fopen("outputWD.txt", "w");
    if (file == NULL) {
        perror("Error opening the file");
        exit(1);
    }

    sleep(1);

    char datareaded[200];
    if (readSecure("log.txt", datareaded,1) == -1) {
        perror("Error reading the log file");
        fclose(file);
        exit(1);
    }


    // Parse the data and assign roles
    char *token = strtok(datareaded, ",");
    while (token != NULL) {
        char type = token[0];          // Get the prefix
        int number = atoi(token + 1);  // Convert the number part to int

        if (type == 'i') {
            pid[input] = number;
        } else if (type == 'd') {
            pid[drone] = number;
        } else if (type == 'o') {
            pid[obstacle] = number;
        } else if (type == 't') {
            pid[target] = number;
        } else if (type == 'b') {
            pid[blackboard] = number;
        }

        token = strtok(NULL, ",");
    }

    // Write the PID values to the output file
    for (int i = 0; i < PROCESSTOCONTROL; i++) {
        fprintf(file, "pid[%d] = %d\n", i, pid[i]);
    }

    //Close the file
    fclose(file);

    // Optional infinite loop (for debugging)
    while (1) {
        sleep(10);
    }

    return 0;
}
