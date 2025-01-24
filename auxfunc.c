#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "auxfunc.h"
#include <time.h>

int numTarget = 4;
int numObstacle = 9;

const char *moves[] = {"upleft", "up", "upright", "left", "center", "right", "downleft", "down", "downright"};
char jsonBuffer[MAX_FILE_SIZE];

int writeSecure(char* filename, char* data, int numeroRiga, char mode) {
    if (mode != 'o' && mode != 'a') {
        fprintf(stderr, "Modalità non valida. Usa 'o' per overwrite o 'a' per append.\n");
        return -1;
    }

    FILE* file = fopen(filename, "r+");  // Apertura per lettura e scrittura
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

    // Blocca il file per accesso esclusivo
    while (flock(fd, LOCK_EX) == -1) {
        if (errno == EWOULDBLOCK) {
            usleep(100000);  // Pausa di 100 ms
        } else {
            perror("Errore nel blocco del file");
            fclose(file);
            return -1;
        }
    }

    // Legge tutto il file in memoria
    char** righe = NULL;  // Array di righe
    size_t numRighe = 0;  // Numero di righe
    char buffer[1024];    // Buffer per leggere ogni riga

    while (fgets(buffer, sizeof(buffer), file)) {
        righe = realloc(righe, (numRighe + 1) * sizeof(char*));
        if (!righe) {
            perror("Errore nella realloc");
            fclose(file);
            return -1;
        }
        righe[numRighe] = strdup(buffer);  // Duplica la riga letta
        numRighe++;
    }

    // Modifica o aggiunge righe
    if (numeroRiga > numRighe) {
        // Aggiungi righe vuote fino alla riga richiesta
        righe = realloc(righe, numeroRiga * sizeof(char*));
        for (size_t i = numRighe; i < numeroRiga - 1; i++) {
            righe[i] = strdup("\n");  // Righe vuote
        }
        righe[numeroRiga - 1] = strdup(data);  // Nuova riga
        numRighe = numeroRiga;
    } else {
        // Se la riga esiste, modifica in base alla modalità
        if (mode == 'o') {
            // Sovrascrivi il contenuto della riga
            free(righe[numeroRiga - 1]);
            righe[numeroRiga - 1] = strdup(data);
        } else if (mode == 'a') {
            // Rimuovi il newline alla fine della riga esistente
            size_t len = strlen(righe[numeroRiga - 1]);
            if (len > 0 && righe[numeroRiga - 1][len - 1] == '\n') {
                righe[numeroRiga - 1][len - 1] = '\0';
            }
            // Concatena il nuovo testo
            char* nuovoContenuto = malloc(len + strlen(data) + 2);
            if (!nuovoContenuto) {
                perror("Errore nella malloc");
                fclose(file);
                return -1;
            }
            sprintf(nuovoContenuto, "%s%s\n", righe[numeroRiga - 1], data); // Nessuno spazio extra
            free(righe[numeroRiga - 1]);
            righe[numeroRiga - 1] = nuovoContenuto;
        }
    }

    // Riscrive il contenuto nel file
    rewind(file);
    for (size_t i = 0; i < numRighe; i++) {
        fprintf(file, "%s", righe[i]);
        if (righe[i][strlen(righe[i]) - 1] != '\n') {
            fprintf(file, "\n");  // Aggiungi newline se mancante
        }
        free(righe[i]);  // Libera la memoria per ogni riga
    }
    free(righe);  // Libera l'array di righe

    // Trunca il file a lunghezza corrente
    if (ftruncate(fd, ftell(file)) == -1) {
        perror("Errore nel troncamento del file");
        fclose(file);
        return -1;
    }

    fflush(file);

    // Sblocca il file
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore nello sblocco del file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int readSecure(char* filename, char* data, int numeroRiga) {
    FILE *file = fopen(filename, "r");
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

    // Blocca il file per lettura condivisa
    while (flock(fd, LOCK_SH) == -1) {
        if (errno == EWOULDBLOCK) {
            usleep(100000);  // Pausa di 100 ms
        } else {
            perror("Errore nel blocco del file");
            fclose(file);
            return -1;
        }
    }

    // Leggi fino alla riga richiesta
    int rigaCorrente = 1;
    char buffer[1024];  // Buffer temporaneo per leggere le righe
    while (fgets(buffer, sizeof(buffer), file)) {
        if (rigaCorrente == numeroRiga) {
            // Copia la riga nel buffer di output
            strncpy(data, buffer, 1024);
            data[1023] = '\0';  // Assicurati che sia terminata correttamente
            break;
        }
        rigaCorrente++;
    }

    // Controlla se abbiamo raggiunto la riga desiderata
    if (rigaCorrente < numeroRiga) {
        fprintf(stderr, "Errore: Riga %d non trovata nel file.\n", numeroRiga);
        flock(fd, LOCK_UN);
        fclose(file);
        return -1;
    }

    // Sblocca il file
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore nello sblocco del file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

void msgUnpack(Message* msgIn, Message* msgOut){
    msgOut->msg = msgIn->msg;
    msgOut->level = msgIn->level;
    msgOut->difficulty = msgIn->difficulty;
    msgOut->drone = msgIn->drone;
    msgOut->targets = msgIn->targets;
    msgOut->obstacles = msgIn->obstacles;
}

void writeMsg(int pipeFds, Message* msg, char* error, FILE* file){
  if (write(pipeFds, msg, sizeof(Message)) == -1) {
        fprintf(file,"Errore%s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }  
}

void inputMsgUnpack(inputMessage* msgIn, inputMessage* msgOut) {
    msgOut->msg = msgOut->msg;
    strncpy(msgOut->name, msgIn->name, MAX_LINE_LENGTH);
    strncpy(msgOut->input, msgIn->input, sizeof(msgOut->input));
    msgOut->difficulty = msgIn->difficulty;
    msgOut->level = msgIn->level;
    msgOut->droneInfo = msgIn->droneInfo;
}



void readMsg(int pipeFds, Message* msgIn, Message* msgOut, char* error, FILE* file){
    if (read(pipeFds, msgIn, sizeof(Message)) == -1){
        fprintf(file, "Errore%s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }

    msgUnpack(msgIn, msgOut);
}

void writeInputMsg(int pipeFds, inputMessage* msg, char* error, FILE* file){
    if (write(pipeFds, msg, sizeof(inputMessage)) == -1) {
        fprintf(file,"Errore%s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }  
}
void readInputMsg(int pipeFds, inputMessage* msgIn, inputMessage* msgOut, char* error, FILE* file){
    if (read(pipeFds, msgIn, sizeof(inputMessage)) == -1){
        fprintf(file, "Errore%s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }

    inputMsgUnpack(msgIn, msgOut);
}

void fdsRead (int argc, char* argv[], int* fds){
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }

     // FDs reading
    char *fd_str = argv[1];
    int index = 0;

    char *token = strtok(fd_str, ",");
    token = strtok(NULL, ",");

    // FDs extraction
    while (token != NULL && index < 4) {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }
}

int writePid(char* file, char mode, int row, char id){

    int pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "%c%d,",id, pid);

    if (writeSecure(file, dataWrite, row, mode) == -1) {
        perror("Error in writing in log.txt");
        exit(1);
    }

    return pid;
}

void handler(int id, FILE *file) {

    char log_entry[256];
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(log_entry, sizeof(log_entry), "%H:%M:%S", timeinfo);
    //fprintf(file, "Process %d received signal from WD at %s\n", id, log_entry);
    //fflush(file);
    writeSecure("log.txt", log_entry, id + 3, 'o');
}