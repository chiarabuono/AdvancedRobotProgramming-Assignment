#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "auxfunc.h"

const char *moves[] = {"upleft", "up", "upright", "left", "center", "right", "downleft", "down", "downright"};


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


void fromStringtoDrone(Drone_bb *drone, const char *drone_str, FILE *file) {
    if (sscanf(drone_str, "%d;%d", &drone->x, &drone->y) != 2) {
        fprintf(file, "Error parsing drone position: %s\n", drone_str);
        fflush(file);
        exit(EXIT_FAILURE);
    }
}

void fromStringtoForce(Force *force, const char *force_str, FILE *file) {
    if (sscanf(force_str, "%f;%f", &force->x, &force->y) != 2) {
        fprintf(file, "Error parsing force: %s\n", force_str);
        fflush(file);
        exit(EXIT_FAILURE);
    }
}

// in obstacles: receive drone position and target positions
void fromStringtoPositions(Drone_bb *drone, int *x, int *y, const char *str, FILE *file) {
    char *token;
    char *str_copy = strdup(str);  // Make a modifiable copy of the string
    char *saveptr;
    int i = 0;

    // Split by semicolon (;)
    token = strtok_r(str_copy, ";", &saveptr);
    while (token != NULL) {
        if (i == 0) {
            // Parse first semicolon-separated part into drone.x and drone.y
            if (sscanf(token, "%d", &drone->x) != 1) {
                fprintf(file, "Error parsing drone x positions: %s\n", token);
                fflush(file);
                free(str_copy);
                exit(EXIT_FAILURE);
            }
        } else if (i == 1) {
            if (sscanf(token, "%d", &drone->y) != 1) {
                fprintf(file, "Error parsing drone y positions: %s\n", token);
                fflush(file);
                free(str_copy);
                exit(EXIT_FAILURE);
            }

        } else if (i == 2) {
            // Parse second semicolon-separated part into x array
            int j = 0;
            char *subtoken, *subsaveptr;
            subtoken = strtok_r(token, ",", &subsaveptr);
            while (subtoken != NULL) {
                x[j++] = atoi(subtoken);
                subtoken = strtok_r(NULL, ",", &subsaveptr);
            }
        } else if (i == 3) {
            // Parse third semicolon-separated part into y array
            int k = 0;
            char *subtoken, *subsaveptr;
            subtoken = strtok_r(token, ",", &subsaveptr);
            while (subtoken != NULL) {
                y[k++] = atoi(subtoken);
                subtoken = strtok_r(NULL, ",", &subsaveptr);
            }
        } 

        // Move to the next part
        token = strtok_r(NULL, ";", &saveptr);
        i++;
    }

    free(str_copy);  // Free the dynamically allocated memory

}

void fromStringtoPositionsWithTwoTargets(int *x1, int *y1, int *x2, int *y2, const char *str, FILE *file) {
    char *token;
    char *str_copy = strdup(str);  // Make a modifiable copy of the string
    char *saveptr;
    int i = 0;

    // Split by semicolon (;)
    token = strtok_r(str_copy, ";", &saveptr);
    while (token != NULL) {
        if (i == 0) {
            // Parse second semicolon-separated part into x array
            int j = 0;
            char *subtoken, *subsaveptr;
            subtoken = strtok_r(token, ",", &subsaveptr);
            while (subtoken != NULL) {
                x1[j++] = atoi(subtoken);
                subtoken = strtok_r(NULL, ",", &subsaveptr);
            }
        } else if (i == 1) {
            // Parse third semicolon-separated part into y array
            int k = 0;
            char *subtoken, *subsaveptr;
            subtoken = strtok_r(token, ",", &subsaveptr);
            while (subtoken != NULL) {
                y1[k++] = atoi(subtoken);
                subtoken = strtok_r(NULL, ",", &subsaveptr);
            }
        } else if (i == 2) {
            // Parse third semicolon-separated part into y array
            int k = 0;
            char *subtoken, *subsaveptr;
            subtoken = strtok_r(token, ",", &subsaveptr);
            while (subtoken != NULL) {
                x2[k++] = atoi(subtoken);
                subtoken = strtok_r(NULL, ",", &subsaveptr);
            }
        } else if (i == 3) {
            // Parse third semicolon-separated part into y array
            int k = 0;
            char *subtoken, *subsaveptr;
            subtoken = strtok_r(token, ",", &subsaveptr);
            while (subtoken != NULL) {
                y2[k++] = atoi(subtoken);
                subtoken = strtok_r(NULL, ",", &subsaveptr);
            }
        } 

        // Move to the next part
        token = strtok_r(NULL, ";", &saveptr);
        i++;
    }

    free(str_copy);  // Free the dynamically allocated memory

}

// in BB: send drone position and target positions to OB
void fromPositiontoString(int *x, int *y, int len, char *str, size_t str_size, FILE *file) {
    int written = 0;

    // Write drone's position
    // written += snprintf(str + written, str_size - written, "%d,%d", drone->x, drone->y);
    // if (written < 0 || written >= str_size) {
    //     fprintf(file, "Error: buffer size exceeded when writing drone position\n");
    //     fflush(file);
    //     return;
    // }

    // Add semicolon separator
    // if (str_size - written > 1) {
    //     str[written++] = ';';
    // } else {
    //     fprintf(file, "Error: buffer size exceeded when adding separator\n");
    //     fflush(file);
    //     return;
    // }

    // Write x array
    for (int i = 0; i < len; i++) {
        if (i > 0) {
            written += snprintf(str + written, str_size - written, ",");
        }
        written += snprintf(str + written, str_size - written, "%d", x[i]);
        if (written < 0 || written >= str_size) {
            fprintf(file, "Error: buffer size exceeded when writing x array\n");
            fflush(file);
            return;
        }
    }

    // Add semicolon separator
    if (str_size - written > 1) {
        str[written++] = ';';
    } else {
        fprintf(file, "Error: buffer size exceeded when adding separator\n");
        fflush(file);
        exit(EXIT_FAILURE);  
    }

    // Write y array
    for (int i = 0; i < len; i++) {
        if (i > 0) {
            written += snprintf(str + written, str_size - written, ",");
        }
        written += snprintf(str + written, str_size - written, "%d", y[i]);
        if (written < 0 || written >= str_size) {
            fprintf(file, "Error: buffer size exceeded when writing y array\n");
            fflush(file);
            exit(EXIT_FAILURE);  
        }
    }
}

void concatenateStr(const char *str1, const char *str2, char *output, size_t output_size, FILE *file) {
    if (snprintf(output, output_size, "%s;%s", str1, str2) >= output_size) {
        fprintf(file, "Error: concatenated string exceeds buffer size\n");
        fflush(file);
        exit(EXIT_FAILURE);  
    }
}


// QUESTA FUNZIONE CONTIENE LA POSSIBILITÀ DI FARE ERASE MA DA PROBLEMI,IL PRIMO PROCESSO CHE PROVA A SCRIVERE NON SCRIVE

// int writeSecure(char* filename, char* data, int numeroRiga, char mode) {
//     if (mode != 'o' && mode != 'a' && mode != 'e') {
//         fprintf(stderr, "Modalità non valida. Usa 'o' per overwrite, 'a' per append o 'e' per erase.\n");
//         return -1;
//     }

//     FILE* file = fopen(filename, "r+");  // Apertura per lettura e scrittura
//     if (file == NULL) {
//         perror("Errore nell'apertura del file");
//         return -1;
//     }

//     int fd = fileno(file);
//     if (fd == -1) {
//         perror("Errore nel recupero del file descriptor");
//         fclose(file);
//         return -1;
//     }

//     // Blocca il file per accesso esclusivo
//     while (flock(fd, LOCK_EX) == -1) {
//         if (errno == EWOULDBLOCK) {
//             usleep(100000);  // Pausa di 100 ms
//         } else {
//             perror("Errore nel blocco del file");
//             fclose(file);
//             return -1;
//         }
//     }

//     // Legge tutto il file in memoria
//     char** righe = NULL;  // Array di righe
//     size_t numRighe = 0;  // Numero di righe
//     char buffer[1024];    // Buffer per leggere ogni riga

//     while (fgets(buffer, sizeof(buffer), file)) {
//         righe = realloc(righe, (numRighe + 1) * sizeof(char*));
//         if (!righe) {
//             perror("Errore nella realloc");
//             fclose(file);
//             return -1;
//         }
//         righe[numRighe] = strdup(buffer);  // Duplica la riga letta
//         numRighe++;
//     }

//     // Gestisci la modalità 'e' (erase)
//     if (mode == 'e') {
//         // Elimina la riga specificata
//         if (numeroRiga > 0 && numeroRiga <= numRighe) {
//             free(righe[numeroRiga - 1]);  // Libera la riga da eliminare
//             for (size_t i = numeroRiga - 1; i < numRighe - 1; i++) {
//                 righe[i] = righe[i + 1];  // Sposta le righe successive
//             }
//             numRighe--;  // Riduce il numero di righe
//         } else {
//             fprintf(stderr, "Errore: Riga %d non trovata per cancellazione.\n", numeroRiga);
//             flock(fd, LOCK_UN);
//             fclose(file);
//             return -1;
//         }
//     } else if (numeroRiga > numRighe) {
//         // Aggiungi righe vuote fino alla riga richiesta
//         righe = realloc(righe, numeroRiga * sizeof(char*));
//         for (size_t i = numRighe; i < numeroRiga - 1; i++) {
//             righe[i] = strdup("\n");  // Righe vuote
//         }
//         righe[numeroRiga - 1] = (mode == 'o') ? strdup(data) : strdup("\n");
//         numRighe = numeroRiga;
//     } else {
//         // Sovrascrivi o aggiungi in base alla modalità
//         if (mode == 'o') {
//             free(righe[numeroRiga - 1]);
//             righe[numeroRiga - 1] = strdup(data);
//         } else if (mode == 'a') {
//             size_t len = strlen(righe[numeroRiga - 1]);
//             if (len > 0 && righe[numeroRiga - 1][len - 1] == '\n') {
//                 righe[numeroRiga - 1][len - 1] = '\0';
//             }
//             char* nuovoContenuto = malloc(len + strlen(data) + 2);
//             if (!nuovoContenuto) {
//                 perror("Errore nella malloc");
//                 fclose(file);
//                 return -1;
//             }
//             sprintf(nuovoContenuto, "%s%s\n", righe[numeroRiga - 1], data);
//             free(righe[numeroRiga - 1]);
//             righe[numeroRiga - 1] = nuovoContenuto;
//         }
//     }

//     // Riscrive il contenuto nel file
//     FILE* tmpFile = fopen("tempfile.txt", "w");  // Creazione del file temporaneo
//     if (!tmpFile) {
//         perror("Errore nella creazione del file temporaneo");
//         flock(fd, LOCK_UN);
//         fclose(file);
//         return -1;
//     }

//     for (size_t i = 0; i < numRighe; i++) {
//         fprintf(tmpFile, "%s", righe[i]);
//         free(righe[i]);  // Libera la memoria per ogni riga
//     }

//     free(righe);  // Libera l'array di righe

//     fclose(tmpFile);  // Chiudi il file temporaneo

//     // Truncare il file originale
//     if (remove(filename) != 0) {
//         perror("Errore nell'eliminazione del file originale");
//         flock(fd, LOCK_UN);
//         fclose(file);
//         return -1;
//     }

//     if (rename("tempfile.txt", filename) != 0) {
//         perror("Errore nel rinominare il file temporaneo");
//         flock(fd, LOCK_UN);
//         fclose(file);
//         return -1;
//     }

//     fflush(file);

//     // Sblocca il file
//     if (flock(fd, LOCK_UN) == -1) {
//         perror("Errore nello sblocco del file");
//         fclose(file);
//         return -1;
//     }

//     fclose(file);
//     return 0;
// }

void handler(int id, int sleep) {

    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry),"%d",sleep);
    writeSecure("log.txt", log_entry, id + 2, 'o');
}