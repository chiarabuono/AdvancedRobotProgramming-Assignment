#include <ncurses.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define w1 0        
#define w2 1        
#define r1 2        
#define r2 3        

#define ask 0       
#define receive 1

#define read 0
#define write 1 

int main() {

    int pipes [4][2][2];

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++){
            if (pipe(pipes[i][j]) == -1) {
                perror("Pipe creation error");
                exit(1);
            }
        }
    }

    pid_t pid1, pid2, pid3, pid4;

    

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}

