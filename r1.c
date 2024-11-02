#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }

    // Prendiamo la stringa dei descrittori passata come argomento
    char *fd_str = argv[1];
    
    // Apriamo il file output.txt in modalità scrittura
    FILE *file = fopen("outputr1.txt", "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    // Scriviamo fd_str nel file invece che sul terminale
    fprintf(file, "fd_str ricevuto r1: %s\n", fd_str);

    // Chiudiamo il file
    fclose(file);

    // Variabile per memorizzare i descrittori di file
    int fds[4]; 
    int index = 0;

    // Usa strtok per separare fd_str, scartando il primo elemento
    char *token = strtok(fd_str, ",");
    token = strtok(NULL, ",");  // Inizia dal secondo elemento

    // Estrai i descrittori di file rimanenti
    while (token != NULL && index < 4) {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }

    return 0;
}
