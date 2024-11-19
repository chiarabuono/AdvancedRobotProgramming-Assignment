#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "auxfunc.h"
#include <signal.h>
#include <time.h>

#define PROCESSTOCONTROL 5

int pid[PROCESSTOCONTROL] = {0};  // Initialize PIDs to 0

struct timeval start, end;
long elapsed_ms;

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
                if(writeSecure("log.txt", "e", i + 2, 'o') == -1){
                    perror("Error writing the log file");
                    fclose(file);
                    exit(1);
                }
                gettimeofday(&start, NULL); // Tempo iniziale
                if (kill(pid[i], SIGUSR1) == -1) {
                    printf("Process %d is not responding or has terminated\n", pid[i]);
                    pid[i] = 0; // Imposta il PID a 0 per ignorare in futuro
                } else {
                    if(readSecure("log.txt", datareaded, i + 2) == -1){
                        perror("Error reading the log file");
                        fclose(file);
                        exit(1);
                    }
                    while(datareaded[0] == 'e'){
                        if(readSecure("log.txt", datareaded, i + 2) == -1){
                            perror("Error reading the log file");
                            fclose(file);
                            usleep(10000);
                            exit(1);
                        }
                    }

                    int period = atoi(datareaded);
                    gettimeofday(&end, NULL); // Tempo iniziale


                    elapsed_ms = 0; 
                    elapsed_ms += (end.tv_usec - start.tv_usec); // Microsecondi in ms

                    if(elapsed_ms < (period*1000)){
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

                        char output[80];
                        snprintf(output, sizeof(output), "%s responded in %ld ns, period: %d ms -> OK", process, elapsed_ms, period);

                        if(writeSecure("outputWD.txt", output, i + 2, 'o') == -1){
                            perror("Error writing the output file");
                            fclose(file);
                            exit(1);
                        }  
                    } else{
                        printf("Process %d is not responding or has terminated\n", pid[i]);
                    }                    
                }
            }
        }

    }
    
    //Close the file
    fclose(file);

    return 0;
}
