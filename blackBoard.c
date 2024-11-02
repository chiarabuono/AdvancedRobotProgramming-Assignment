#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define w1 0        
#define w2 1        
#define r1 2        
#define r2 3        

#define ask 0       
#define receive 1

#define read 0
#define write 1 

#define PROCESSNUM 4
#define PIPESNUM 4

int main() {

    int pipes [PIPESNUM][2][2];

    for (int i = 0; i < PIPESNUM; i++) {
        for (int j = 0; j < 2; j++){
            if (pipe(pipes[i][j]) == -1) {
                perror("Pipe creation error");
                exit(1);
            }
        }
    }

    pid_t pids [PROCESSNUM];
    char fd_str[15];

    sprintf(fd_str, "%d", pipes[w1][ask][read]);
    char *args[] = {fd_str};

    // for (int i = 0; i < PROCESSNUM; i++){
        pids[1] = fork();
        if(pids[1] != 0){
            return pids[1];
        }
        else{
            execl("./w1", "./w1", fd_str, NULL);
        }
    // }

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}

