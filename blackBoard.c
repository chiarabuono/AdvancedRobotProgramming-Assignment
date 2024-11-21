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

// process to whom that asked or received
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define nfds 19

int pid;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(BLACKBOARD,100);
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
    close(fds[DRONE][askwr]);
    close(fds[DRONE][recrd]);
    close(fds[INPUT][askwr]);
    close(fds[INPUT][recrd]);
    close(fds[OBSTACLE][askwr]);
    close(fds[OBSTACLE][recrd]);
    close(fds[TARGET][askwr]);
    close(fds[TARGET][recrd]);

    // Reading buffer
    char data[80];
    ssize_t bytesRead;
    fd_set readfds;
    struct timeval tv;

    //Setting select timeout
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    
    signal(SIGUSR1, sig_handler);

    //Drone_bb drone;
    //Force force_o, force_t;
    char drone_str[80];
    char forceO_str[80];
    char forceT_str[80];

    while (1) {
        // receiving drone position and sending to obstacle.c and target.c
        // fprintf(file, "Reading drone position. Before %d, %d\n",  drone.x, drone.y);
        // fflush(file);
        
        if (read(fds[DRONE][askrd], &drone_str, sizeof(drone_str)) == -1){
            fprintf(file,"[BB] Error reading drone position\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }

        fprintf(file, "Sending drone position to [OB]\n");
        fflush(file);
        if (write(fds[OBSTACLE][recwr], &drone_str, sizeof(drone_str)) == -1) {
            fprintf(file,"[BB] Error sending drone position to [OBSTACLE]\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }

        fprintf(file, "Sending drone position to [TA]\n");
        fflush(file);
        if (write(fds[TARGET][recwr], &drone_str, sizeof(drone_str)) == -1) {
            fprintf(file,"[BB] Error sending drone position to [TARGET]\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }

        usleep(100);

        // receiving force from obstacle.c and target.c and sending to drone
        fprintf(file, "Reading force_o \n");
        fflush(file);
        if (read(fds[OBSTACLE][askrd], &forceO_str, sizeof(forceO_str)) == -1){
            fprintf(file,"[BB] Error reading force_o\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }
        fprintf(file, "Reading force_t \n");
        fflush(file);
        if (read(fds[TARGET][askrd], &forceT_str, sizeof(forceT_str)) == -1){
            fprintf(file,"[BB] Error reading force_t\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }

        fprintf(file, "Sending force_o \n");
        fflush(file);
        if (write(fds[DRONE][recwr], &forceO_str, sizeof(forceO_str)) == -1) {
            fprintf(file,"[BB] error sending force_o to [DRONE]\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }
        fprintf(file, "Reading force_t \n");
        fflush(file);
        if (write(fds[DRONE][recwr], &forceT_str, sizeof(forceT_str)) == -1) {
            fprintf(file,"[BB] error sending force_t to [DRONE]\n");
            fflush(file);
            exit(EXIT_FAILURE);
        }


        //sleep(1);
            
        //FDs setting for select

        // FD_ZERO(&readfds);
        // FD_SET(fds[DRONE][askrd], &readfds);
        // FD_SET(fds[INPUT][askrd], &readfds);
        // FD_SET(fds[OBSTACLE][askrd], &readfds);
        // FD_SET(fds[TARGET][askrd], &readfds); 

        // int fdsQueue [4];
        // int ready = 0;

        // int sel = select(nfds, &readfds, NULL, NULL, &tv);

        // if (sel == -1) {
        //     perror("Select error");
        //     break;
        // } 

        // if (FD_ISSET(fds[DRONE][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[DRONE][askrd];
        //     ready++;
        // }
        // if (FD_ISSET(fds[INPUT][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[INPUT][askrd];
        //     ready++;
        // }
        // if (FD_ISSET(fds[OBSTACLE][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[OBSTACLE][askrd];
        //     ready++;
        // }
        // if (FD_ISSET(fds[TARGET][askrd], &readfds)) {
        //     fdsQueue[ready] = fds[TARGET][askrd];
        //     ready++;
        // }
        

        // if(ready > 0){
        //     unsigned int rand = randomSelect(ready);
        //     int selected = fdsQueue[rand];

        //     if (selected == fds[DRONE][askrd]){
        //         fprintf(file, "selected drone\n");
        //         fflush(file);   
        //     } else if (selected == fds[INPUT][askrd]){
        //         fprintf(file, "selected input\n");
        //         fflush(file);             
        //     } else if (selected == fds[OBSTACLE][askrd]){
        //         fprintf(file, "selected obstacle\n");
        //         fflush(file);
        //     }else if (selected == fds[TARGET][askrd]){
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