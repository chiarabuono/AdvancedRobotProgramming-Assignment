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

int pids[PROCESSTOCONTROL] = {0};  // Initialize PIDs to 0

struct timeval start, end;
long elapsed_ms;

FILE *file;

void sig_handler(int signo) {
    if(signo == SIGTERM){
        fprintf(file, "Watchdog is quitting\n");
        fflush(file);   
        fclose(file);
        exit(EXIT_SUCCESS);
    }
}

void closeAll(int id){
    for(int i  = 0; i < PROCESSTOCONTROL; i++){
        if (i != id) {
            if (kill(pids[i], SIGTERM) == -1) {
                fprintf(file,"Process %d is not responding or has terminated\n", pids[i]);
                fflush(file);
            }
        }
    }
    fprintf(file, "Watchdog is quitting all because %d is dead\n", id);
    fflush(file);
    fclose(file);
    exit(EXIT_SUCCESS);
}

int main() {
    // Open the output file for writing
    file = fopen("log/outputWD.txt", "w");
    if (file == NULL) {
        perror("Error opening the file");
        exit(1);
    }

    int pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "w%d,", pid);

    if(writeSecure("log/log.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    sleep(1);

    char datareaded[200];
    if (readSecure("log/log.txt", datareaded,1) == -1) {
        perror("Error reading the log file");
        exit(1);
    }

    // Parse the data and assign roles
    char *token = strtok(datareaded, ",");
    while (token != NULL) {
        char type = token[0];          // Get the prefix
        int number = atoi(token + 1);  // Convert the number part to int

        if (type == 'i') {
            pids[INPUT] = number;
        } else if (type == 'd') {
            pids[DRONE] = number;
        } else if (type == 'o') {
            pids[OBSTACLE] = number;
        } else if (type == 't') {
            pids[TARGET] = number;
        } else if (type == 'b') {
            pids[BLACKBOARD] = number;
        }else{
            ;
        }

        token = strtok(NULL, ",");
    }

    // Write the PID values to the output file
    for (int i = 0; i < PROCESSTOCONTROL; i++) {
        fprintf(file, "pid[%d] = %d\n", i, pids[i]);
        fflush(file);
    }

    signal(SIGTERM, sig_handler);

    for (int i = 0; i < PROCESSTOCONTROL; i++) {
            if (kill(pids[i], SIGUSR1) == -1) {
                fprintf(file,"Process %d is not responding or has terminated\n", pids[i]);
                fflush(file);
                closeAll(i);
            }
        usleep(10000);
    }

    int interval = 0;

    while (1) {

        sleep(1);
        interval++;

        if(interval >= 4){
            interval = 0;
            for (int i = 0; i < PROCESSTOCONTROL; i++) {
                    if (kill(pids[i], SIGUSR1) == -1) {
                        fprintf(file,"Process %d is not responding or has terminated\n", pids[i]);
                        fflush(file);
                        closeAll(i);
                    }
                usleep(10000);
            }
        }   

        usleep(10000);
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        char timeReaded[80];
        for(int i = 0; i < PROCESSTOCONTROL; i++){
            if(readSecure("log/log.txt", timeReaded, i + 3) == -1){
                perror("Error reading the log file");
                fclose(file);
                exit(1);
            }

            int hours, minutes, seconds;
            sscanf(timeReaded, "%d:%d:%d", &hours, &minutes, &seconds);
            long timeReadedInSeconds = hours * 3600 + minutes * 60 + seconds;
            
            long currentTimeInSeconds = timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;
            long timeDifference = currentTimeInSeconds - timeReadedInSeconds;

            if (timeDifference > 5) {
                closeAll(i);
            }
        } 
    }                 
    
    //Close the file
    fclose(file);

    return 0;
}
