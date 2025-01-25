#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 100
#define USE_MACRO_LOG 1

char difficultyStr[10];
// Definizione del file globale per il logging
static FILE *logFile = NULL;


// Funzione per chiudere il file di log
static inline void closeLogFile() {
    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }
}

// Macro per il logging della configurazione
#if USE_MACRO_LOG
#define LOGCONFIG(status)                                      \
    {                                                          \
        if (logFile) {                                         \
            fprintf(logFile, "Player Name: %s\n", status.name); \
            if(status.difficulty == 1){ \
                strcpy(difficultyStr,"Easy");   \
            } else if(status.difficulty == 2){  \
                strcpy(difficultyStr,"Hard");   \
            }   \
            fprintf(logFile, "Difficulty: %s\n", difficultyStr);  \
            fprintf(logFile, "Level: %d\n", status.level); \
            fflush(logFile);\
        } else {                                               \
            printf("Log file is not initialized.\n"); \
            fflush(stdout);\
        }                                                      \
    }
#endif

// Macro per un log più dettagliato
#ifdef LOGCONFIG
#define LOG(status)                                             \
    {                                                         \
        fprintf(logFile,"[%s,%s] at %d,%s:\n", __DATE__,__TIME__,__LINE__, __FILE__);          \
        fflush(logFile);    \
        LOGCONFIG(status);                                      \
    }
#endif

// Macro NEWMAP (definizione dummy per esempio)
#define NEWMAP() {                                                          \
        if (logFile) {                                         \
            fprintf(logFile, "New map created\n"); \
        } else {                                               \
            fprintf(stderr, "Log file is not initialized.\n"); \
        }                                                      \
    }

#endif // LOG_H
