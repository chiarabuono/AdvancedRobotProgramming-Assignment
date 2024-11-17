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

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

int pid;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(target,pid,target+2);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    FILE *file = fopen("outputtarget.txt", "a");
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
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "t%d,", pid);

    if(writeSecure("log.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    while (1) {

       pause();
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
