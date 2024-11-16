#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>

int writeSecure(char* filename, char* data){

    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return -1;
    }

    int fd = fileno(file);
    if (fd == -1) {
        perror("Errore nel recupero del file descriptor");
        fclose(file);
        return -1;
    }

    while (flock(fd, LOCK_EX) == -1) {
        if (errno == EWOULDBLOCK > 0) {
            // Il lock non è stato acquisito perché è già in uso, ritenta dopo un breve ritardo
            usleep(100000);  // Pausa di 100 ms (100000 microsecondi)
        } else {
            perror("Errore nel blocco del file");
            fclose(file);
            return -1;
        }
    }
    
    fprintf(file, "%s", data);
    fflush(file);
    
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore nello sblocco del file");
        return -1;
    }

    fclose(file);
    return 0;
}
