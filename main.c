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
#define PIPEXPROCESS 2

int main() {

    int pipes [PROCESSNUM][PIPEXPROCESS][2];

    // creazione e riempimento pipes
    for (int i = 0; i < PROCESSNUM; i++) {
        for (int j = 0; j < 2; j++){
            if (pipe(pipes[i][j]) == -1) {          //creazione di una pipe con il comando pipe(number)
                perror("Pipe creation error");      // return error
                exit(1);
            } else {printf("read: %d\t write: %d\n", pipes[i][j][0], pipes[i][j][1]);}  // per capire fd delle varie pipe
        }
    }

    
    char fd_str[PROCESSNUM][50];        // file descriptor of pipes (ex. read 3 - write 4, read 5 - write 6)
    char test[15];                      // temporary variable
    

    for(int i = 0; i < PROCESSNUM; i++){    //initial 0 to be able to use strcat
        sprintf(fd_str[i], "%d", 0);
        strcat(fd_str[i], ",");
    }

    //riempimento fd_str con i valori in pipes (separati da virgola)
    for(int i = 0; i < PROCESSNUM; i++){
        for(int j = 0; j < PIPEXPROCESS; j++){
            for(int k = 0; k < 2; k++){
                sprintf(test, "%d", pipes[i][j][k]);
                strcat(fd_str[i], test);
                strcat(fd_str[i], ",");
            }
        }
    }

    pid_t pids[PROCESSNUM];             // process ID

    // riempimento pids[PROCESSNUM]
    for (int i = 0; i < PROCESSNUM; i++){
        pids[i] = fork();

        //creation error
        if (pids[i] < 0) {
            perror("Errore nella fork");
            exit(1);
        } 
        // va a buon fine
        else if (pids[i] == 0) { // Processo figlio
            char *args[] = { NULL, fd_str[i], NULL }; // Array di argomenti per execvp

            switch (i) {
                case 0:
                    args[0] = "./w1";
                    if (execvp(args[0], args) == -1) {
                        perror("Errore in execvp per w1");
                        exit(1);
                    }
                    break;
                case 1:
                    args[0] = "./w2";
                    if (execvp(args[0], args) == -1) {
                        perror("Errore in execvp per w2");
                        exit(1);
                    }
                    break;
                case 2:
                    args[0] = "./r1";
                    if (execvp(args[0], args) == -1) {
                        perror("Errore in execvp per r1");
                        exit(1);
                    }
                    break;
                case 3:
                    args[0] = "./r2";
                    if (execvp(args[0], args) == -1) {
                        perror("Errore in execvp per r2");
                        exit(1);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    // Fork per blackBoard
    pid_t pidbb = fork();

    if (pidbb < 0) {
        perror("Errore nella fork");
        exit(1);
    } 
    else if (pidbb == 0) { 
        // Definiamo gli argomenti per blackBoard
        char *args[] = { "./blackBoard", fd_str[0], fd_str[1], fd_str[2], fd_str[3], NULL };
        
        // Eseguiamo blackBoard
        if (execvp(args[0], args) == -1) {
            perror("Errore in execvp per blackBoard");
            exit(1);
        }
    }

    // Attende tutti i processi figli
    for (int i = 0; i < PROCESSNUM + 1; i++) { // PROCESSNUM + 1 per includere blackBoard
        wait(NULL);
    }

    return 0;
}
