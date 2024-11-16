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

int main(int argc, char *argv[]) {

    // Apriamo il file output.txt in modalità scrittura
    FILE *file = fopen("outpputWD.txt", "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }


    // Chiudiamo il file
    fclose(file);
    return 0;
}
