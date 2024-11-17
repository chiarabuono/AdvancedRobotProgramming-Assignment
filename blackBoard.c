#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "auxfunc.h"
#include <signal.h>

#define drone 0
#define input 1
#define obstacle 2
#define target 3

// process to whom that asked or received
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define nfds 19

int pid;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(blackboard,pid,blackboard+2);
    }
}

int randomSelect(int n) {
    unsigned int random_number;
    int random_fd = open("/dev/urandom", O_RDONLY);
    
    if (random_fd == -1) {
        perror("Error opening /dev/urandom");
        return -1;  // Indicate failure
    }

    if (read(random_fd, &random_number, sizeof(random_number)) == -1) {
        perror("Error reading from /dev/urandom");
        close(random_fd);
        return -1;  // Indicate failure
    }
    
    close(random_fd);  // Close file after successful read
    
    return random_number % n;
}


int main(int argc, char *argv[]) {

    // Log file opening
    FILE *file = fopen("outputbb.txt", "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    int fds[4][4] = {0};

    if (argc < 5) {
        fprintf(stderr, "Uso: %s <fd_str[0]> <fd_str[1]> <fd_str[2]> <fd_str[3]>\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        char *fd_str = argv[i + 1];

        int index = 0;

        // Tokenization each value and discard ","
        char *token = strtok(fd_str, ",");
        token = strtok(NULL, ",");

        // FDs ectraction
        while (token != NULL && index < 4) {
            fds[i][index] = atoi(token);
            index++;
            token = strtok(NULL, ",");
        }
    }

    // //FDs print
    //     for (int i = 0; i < 4; i++) {
    //     fprintf(file, "Descrittori di file estratti da fd_str[%d]:\n", i);
    //     for (int j = 0; j < 4; j++) {
    //         fprintf(file, "fds[%d]: %d\n", j, fds[i][j]);
    //     }
    // }

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "b%d,", pid);

    if(writeSecure("log.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    // closing the unused fds to avoid deadlock
    close(fds[drone][askwr]);
    close(fds[drone][recrd]);
    close(fds[input][askwr]);
    close(fds[input][recrd]);
    close(fds[obstacle][askwr]);
    close(fds[obstacle][recrd]);
    close(fds[target][askwr]);
    close(fds[target][recrd]);

    // Reading buffer
    char data[80];
    ssize_t bytesRead;
    fd_set readfds;
    struct timeval tv;

    //Setting select timeout
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    
    signal(SIGUSR1, sig_handler);

    while (1) {
        pause();     
        // //FDs setting for select

        // FD_ZERO(&readfds);
        // FD_SET(fds[drone][askrd], &readfds);
        // FD_SET(fds[input][askrd], &readfds);
        // FD_SET(fds[obstacle][askrd], &readfds);
        // FD_SET(fds[target][askrd], &readfds); 

        // int fdsQueue [4];
        // int ready = 0;

        // int sel = select(nfds, &readfds, NULL, NULL, &tv);

        // if (sel == -1) {
        //     perror("Select error");
        //     break;
        // } 

        // if (FD_ISSET(fds[drone][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[drone][askrd];
        //     ready++;
        // }
        // if (FD_ISSET(fds[input][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[input][askrd];
        //     ready++;
        // }
        // if (FD_ISSET(fds[obstacle][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[obstacle][askrd];
        //     ready++;
        // }
        // if (FD_ISSET(fds[target][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[target][askrd];
        //     ready++;
        // }
        

        // if(ready > 0){
        //     unsigned int rand = randomSelect(ready);
        //     int selected = fdsQueue[rand];

        //     if (selected == fds[drone][askrd]){
        //         fprintf(file, "selected drone\n");
        //         fflush(file);   
        //     } else if (selected == fds[input][askrd]){
        //         fprintf(file, "selected input\n");
        //         fflush(file);             
        //     } else if (selected == fds[obstacle][askrd]){
        //         fprintf(file, "selected obstacle\n");
        //         fflush(file);
        //     }else if (selected == fds[target][askrd]){
        //         fprintf(file, "selected target\n");
        //         fflush(file);
        //     }else{
        //         fprintf(file, "Problems\n");
        //         fflush(file);
        //     }
        // }
    }

    return 0;
}