#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <fd_str[0]> <fd_str[1]> <fd_str[2]> <fd_str[3]>\n", argv[0]);
        exit(1);
    }

    // Apriamo il file outputbb.txt in modalità scrittura
    FILE *file = fopen("outputbb.txt", "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    // Iteriamo su ogni fd_str passato come argomento
    for (int i = 1; i < argc; i++) {
        char *fd_str = argv[i];
        fprintf(file, "\nfd_str ricevuto bb[%d]: %s\n", i-1, fd_str);

        int fds[4];
        int index = 0;

        // Tokenizza ogni fd_str e scarta il primo elemento
        char *token = strtok(fd_str, ",");
        token = strtok(NULL, ",");  // Salta il primo elemento

        // Estrai i descrittori di file rimanenti
        while (token != NULL && index < 4) {
            fds[index] = atoi(token);
            index++;
            token = strtok(NULL, ",");
        }

        // Stampa i descrittori estratti nel file
        fprintf(file, "Descrittori di file estratti da fd_str[%d]:\n", i-1);
        for (int j = 0; j < index; j++) {
            fprintf(file, "fds[%d]: %d\n", j, fds[j]);
        }
    }

    // Chiudiamo il file
    fclose(file);

    return 0;
}
