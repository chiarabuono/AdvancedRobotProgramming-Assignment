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
#include <time.h>

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

    // Optional infinite loop (for debugging)
    while (1) {
        sleep(10);
        for (int i = 0; i < PROCESSTOCONTROL; i++) {
            if (pid[i] != 0) {
                if (kill(pid[i], SIGUSR1) == -1) {
                    printf("Process %d is not responding or has terminated\n", pid[i]);
                    pid[i] = 0; // Imposta il PID a 0 per ignorare in futuro
                } else {
                    usleep(100000);
                    if(readSecure("log.txt", datareaded, i + 2) == -1){
                        perror("Error reading the log file");
                        fclose(file);
                        exit(1);
                    }
                    // Extract time and period from datareaded
                    char timereaded[9];  // Format: hh:mm:ss
                    char period[10]; // Assuming period is less than 10 characters
                    sscanf(datareaded, "%8[^,],%9s", timereaded, period);
                    
                    // Get current time
                    time_t now = time(NULL);
                    struct tm *current_time = localtime(&now);
                    char current_time_str[9];
                    strftime(current_time_str, sizeof(current_time_str), "%H:%M:%S", current_time);

                    // Convert both times to seconds since midnight
                    int h1, m1, s1, h2, m2, s2;
                    sscanf(timereaded, "%d:%d:%d", &h1, &m1, &s1);
                    sscanf(current_time_str, "%d:%d:%d", &h2, &m2, &s2);
                    int time_seconds = h1 * 3600 + m1 * 60 + s1;
                    int current_time_seconds = h2 * 3600 + m2 * 60 + s2;

                    // Convert period to seconds
                    int period_seconds = atoi(period);

                    char process[20];   
                    if(i == 0){
                        strcpy(process, "drone");
                    } else if(i == 1){
                        strcpy(process, "input");
                    } else if(i == 2){
                        strcpy(process, "obstacle");
                    } else if(i == 3){
                        strcpy(process, "target");
                    } else if(i == 4){
                        strcpy(process, "blackboard");
                    }
                    // Check if the difference is less than the period
                    if (abs(current_time_seconds - time_seconds) < period_seconds) {
                        fprintf(file, "%s not blocked\n", process);
                        fflush(file);
                    } else {
                        fprintf(file, "%s blocked\n", process);
                        fflush(file);
                    }
                }
            }
        }

    }
    
    //Close the file
    fclose(file);

    return 0;
}
