#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "auxfunc.h"
#include <signal.h>
#include <cjson/cJSON.h>

// process to whom that asked or received
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define nfds 19

#define HMARGIN 5
#define WMARGIN 5
#define BMARGIN 2
#define SCALERATIO 1

#define PERIODBB 10000  // [us]

#define EASY 1
#define HARD 2

#define MAPRESET 5 // [s]

int nh, nw;
float scaleh = 1.0, scalew = 1.0;

float second = 1000000;

int pid;

char ack [2] = "A\0";

int fds[4][4] = {0};
int mode = PLAY;

WINDOW * win;
WINDOW * map;

FILE *file;
FILE *conffile;

Message status;
Drone_bb prevDrone = {0, 0};

Message msg;
Config config;

int pids[6] = {0};  // Initialize PIDs to 0

int mask[MAX_TARGET] = {0};
int collision = 0;
int targetsHit = 0;

float resetMap = MAPRESET; // [s]
int diffic = EASY;
int score  = 0;
int levelTime = 30;
float elapsedTime = 0;
int remainingTime = 0;
int level = 1;
char name[20];
char diff[10];

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(BLACKBOARD, file);
    } else if (signo == SIGTERM) {
        fprintf(file, "Blackboard is quitting\n");
        fflush(file);
        fclose(file);
        close(fds[DRONE][recwr]);
        close(fds[DRONE][askrd]);
        close(fds[INPUT][recwr]);
        close(fds[INPUT][askrd]);
        close(fds[OBSTACLE][recwr]);
        close(fds[OBSTACLE][askrd]);
        close(fds[TARGET][recwr]);
        close(fds[TARGET][askrd]);
        exit(EXIT_SUCCESS);
    }
}

void storePreviousPosition(Drone_bb *drone) {

    prevDrone.x = drone ->x;
    prevDrone.y = drone ->y;

}

void resizeHandler(int sig){
    getmaxyx(stdscr, nh, nw);  /* get the new screen size */
    scaleh = ((float)(nh - 2) / (float)WINDOW_LENGTH);
    scalew = (float)nw / (float)WINDOW_WIDTH;
    endwin();
    initscr();
    start_color();
    curs_set(0);
    noecho();
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
}

void mapInit(FILE *file){
    fprintf(file, "\n--------------------\n");
    fprintf(file, "MAP INITIALIZATION\n");
    fprintf(file, "--------------------\n");
    fprintf(file, "Waiting for drone position\n");
    fflush(file);

    // read initial drone position
    readMsg(fds[DRONE][askrd], &msg, &status, 
            "[BB] Error reading drone position\n", file);
    
    fprintf(file, "Drone updated position: %d,%d\n", status.drone.x, status.drone.y);
    fflush(file);

    status.level = level;

    // send drone position to target
    writeMsg(fds[TARGET][recwr], &status, 
            "[BB] Error sending drone position to [TARGET]\n", file);
    
    
    // receiving target position
    readMsg(fds[TARGET][askrd], &msg, &status, 
            "[BB] Error reading target\n", file);

    // send drone and target position to obstacle
    writeMsg(fds[OBSTACLE][recwr], &status, 
            "[BB] Error sending drone and target position to [OBSTACLE]\n", file);

    // receiving obstacle position
    readMsg(fds[OBSTACLE][askrd], &msg, &status, 
            "[BB] Error reading obstacles positions\n", file);
    
    //Update drone position
    writeMsg(fds[DRONE][recwr], &status, 
            "[BB] Error sending updated map\n", file);
}

void drawDrone(WINDOW * win){
    int row = (int)(status.drone.y * scaleh);
    int col = (int)(status.drone.x * scalew);
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(1));   
    mvwprintw(win, row - 1, col, "|");     
    mvwprintw(win, row, col + 1, "--");
    mvwprintw(win, row, col, "+");
    mvwprintw(win, row + 1, col, "|");     
    mvwprintw(win, row , col -2, "--");
    wattroff(win, COLOR_PAIR(1)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawObstacle(WINDOW * win){
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(2)); 
    for(int i = 0; i < MAX_OBSTACLES; i++){
        mvwprintw(win, (int)(status.obstacles.y[i]*scaleh), (int)(status.obstacles.x[i]*scalew), "0");
    }
    wattroff(win, COLOR_PAIR(2)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawTarget(WINDOW * win) {
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(3)); 
    for(int i = 0; i < MAX_TARGET; i++){
        if (status.targets.value[i] == 0) continue;
        char val_str[2];
        sprintf(val_str, "%d", status.targets.value[i]); // Converte il valore in stringa
        mvwprintw(win, (int)(status.targets.y[i] * scaleh), (int)(status.targets.x[i] * scalew), "%s", val_str); // Usa un formato esplicito
    } 
    wattroff(win, COLOR_PAIR(3)); 
    wattroff(win, A_BOLD); // Disattiva il grassetto
}

void drawMenu(WINDOW* win) {

    wattron(win, A_BOLD); // Attiva il grassetto

    // Preparazione delle stringhe
    char score_str[10], diff_str[10], time_str[10], level_str[10];
    sprintf(score_str, "%d", score);
    sprintf(diff_str, "%s", diff);
    sprintf(time_str, "%d", remainingTime);
    sprintf(level_str, "%d", level);

    // Array con le etichette e i valori corrispondenti
    const char* labels[] = { "Score: ", "Player: ", "Difficulty: ", "Time: ", "Level: " };
    const char* values[] = { score_str, name, diff_str, time_str, level_str };

    int num_elements = 5; // Numero di elementi nel menu

    // Calcola la lunghezza totale occupata dalle stringhe
    int total_length = 0;
    for (int i = 0; i < num_elements; i++) {
        total_length += strlen(labels[i]) + strlen(values[i]);
    }

    // Calcola lo spazio rimanente e lo spazio tra gli elementi
    int remaining_space = nw - total_length; // Spazio non occupato dalle stringhe
    int spacing = remaining_space / (num_elements + 1); // Spaziatura uniforme

    // Stampa gli elementi equidistanti
    int current_position = spacing; // Posizione iniziale
    for (int i = 0; i < num_elements; i++) {
        // Stampa l'etichetta e il valore
        mvwprintw(win, 0, current_position, "%s%s", labels[i], values[i]);

        // Aggiorna la posizione corrente
        current_position += strlen(labels[i]) + strlen(values[i]) + spacing;
    }
    wattroff(win, A_BOLD); // Disattiva il grassetto
    // Aggiorna la finestra per mostrare i cambiamenti
    wrefresh(win);
}

int randomSelect(int n) {
    unsigned int random_number;
    int random_fd = open("/dev/urandom", O_RDONLY);
    
    if (random_fd == -1) {
        perror("Error opening /dev/urandom");
        return -1;  // Indicate failure
    }

    if (read(random_fd, &random_number, sizeof(random_number)) == -1) {
        perror("Error reading from /dev/urandom");
        close(random_fd);
        return -1;  // Indicate failure
    }
    
    close(random_fd);  // Close file after successful read
    
    return random_number % n;
}

void detectCollision(Drone_bb* drone, Drone_bb * prev, Targets* targets, FILE* file) {
    //fprintf(file, "PREV(%d, %d), DRONE(%d, %d)\n TARGET:\n", prev->x, prev->y, drone->x, drone->y);
    for (int i = 0; i < MAX_TARGET; i++) {
        //fprintf(file, " (%d, %d)", targets->x[i], targets->y[i]);
        if (!mask[i] && (((prev->x <= targets->x[i] + 2 && targets->x[i] - 2 <= drone->x)  &&
            (prev->y <= targets->y[i] + 2 && targets->y[i]- 2 <= drone->y) )||
            ((prev->x >= targets->x[i] - 2 && targets->x[i] >= drone->x + 2) &&
            (prev->y >= targets->y[i] - 2 && targets->y[i] >= drone->y + 2) ))){
                targets->value[i] = 0;
                collision = 1;
                targetsHit++;
                score += targets->value[i];
            }
    }

}

void createNewMap(){

    readMsg(fds[TARGET][askrd], &msg, &status, 
            "[BB] Error reading ready from target", file);
    
    if(status.msg == 'R'){

        fprintf(file, "Sending drone position to [TARGET]\n");
        fflush(file);

        writeMsg(fds[TARGET][recwr], &status, 
            "[BB] Error sending drone position to target", file);

        fprintf(file, "waiting for new targets\n");
        fflush(file);

        readMsg(fds[TARGET][askrd], &msg, &status, 
            "[BB] Error reading target positions", file);

        fprintf(file, "Sending new targets to obstacle");
        fflush(file);

        writeMsg(fds[OBSTACLE][recwr], &status, 
            "[BB] Error sending drone and target position to [OBSTACLE]", file);

        fprintf(file, "Reading obstacles positions from [OB]\n");
        fflush(file);

        readMsg(fds[OBSTACLE][askrd], &msg, &status, 
            "[BB] Reading obstacles positions", file);

        readMsg(fds[DRONE][askrd], &msg, &status, 
            "[BB] Reading ready from [DRONE]", file);

        if(status.msg == 'R'){

            fprintf(file, "Asking drone position to [DRONE]\n");
            fflush(file);

            status.msg = 'M';

            writeMsg(fds[DRONE][recwr], &status, 
                    "[BB] Error asking drone position", file);

            readMsg(fds[DRONE][askrd], &msg, &status, 
                    "[BB] Error reading drone position", file);
        }
    }
}

void readConf(){

    conffile = fopen("appsettings.json", "r");

    if (conffile == NULL) {
        perror("Error opening the file");
        //return EXIT_FAILURE;//1
    }

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), conffile); 

    fclose(conffile);

    cJSON *json = cJSON_Parse(jsonBuffer);// parse the text to json object

    if (json == NULL)
    {
        perror("Error parsing the file");
        //return EXIT_FAILURE;
    }

    conf_ptr gameConfig = malloc(sizeof(conf_ptr)); // allocate memory dinamically
    
    strcpy(gameConfig->difficulty, cJSON_GetObjectItemCaseSensitive(json, "Difficulty")->valuestring);
    strcpy(gameConfig->playerName, cJSON_GetObjectItemCaseSensitive(json, "PlayerName")->valuestring);
    gameConfig->startingLevel = cJSON_GetObjectItemCaseSensitive(json, "StartingLevel")->valueint;
    
    // fprintf(file, "Player name: %s\n", gameConfig->playerName);
    // fprintf(file, "Difficulty: %s\n", gameConfig->difficulty);
    // fprintf(file, "Starting level: %d\n", gameConfig->startingLevel);
    // fflush(file);

    level = gameConfig->startingLevel;
    strcpy(name, gameConfig->playerName);
    strcpy(diff, gameConfig->difficulty);

    cJSON_Delete(json);//clean
    
    free(gameConfig);//malloc --> clean //delete the memory dinamically allocated

}

int main(int argc, char *argv[]) {

    // Log file opening
    file = fopen("outputbb.txt", "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    if (argc < 5) {
        fprintf(stderr, "Uso: %s <fd_str[0]> <fd_str[1]> <fd_str[2]> <fd_str[3]>\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        char *fd_str = argv[i + 1];

        int index = 0;

        // Tokenization each value and discard ","
        char *token = strtok(fd_str, ",");
        token = strtok(NULL, ",");

        // FDs ectraction
        while (token != NULL && index < 4) {
            fds[i][index] = atoi(token);
            index++;
            token = strtok(NULL, ",");
        }
    }

    // //FDs print
    //     for (int i = 0; i < 4; i++) {
    //     fprintf(file, "Descrittori di file estratti da fd_str[%d]:\n", i);
    //     for (int j = 0; j < 4; j++) {
    //         fprintf(file, "fds[%d]: %d\n", j, fds[i][j]);
    //     }
    // }

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "b%d,", pid);

    if(writeSecure("log.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }
    

    // closing the unused fds to avoid deadlock
    close(fds[DRONE][askwr]);
    close(fds[DRONE][recrd]);
    close(fds[INPUT][askwr]);
    close(fds[INPUT][recrd]);
    close(fds[OBSTACLE][askwr]);
    close(fds[OBSTACLE][recrd]);
    close(fds[TARGET][askwr]);
    close(fds[TARGET][recrd]);

    // Reading buffer
    char data[80];
    ssize_t bytesRead;
    fd_set readfds;
    struct timeval tv;
    
    //Setting select timeout
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    signal(SIGUSR1, sig_handler);
    signal(SIGWINCH, resizeHandler);
    signal(SIGTERM, sig_handler);

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
    scaleh = (float)(nh - 2) / (float)WINDOW_LENGTH;
    scalew = (float)nw / (float)WINDOW_WIDTH;

    // Definizione delle coppie di colori
    init_pair(1, COLOR_BLUE, COLOR_BLACK);     // Testo blu su sfondo nero
    init_pair(2, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero
    init_pair(3, COLOR_GREEN, COLOR_BLACK);    // Testo verde su sfondo nero

    usleep(500000);

    char datareaded[200];
    if (readSecure("log.txt", datareaded,1) == -1) {
        perror("Error reading the log file");
        exit(1);
    }

    // Parse the data and assign roles
    char *token = strtok(datareaded, ",");
    while (token != NULL) {
        char type = token[0];          // Get the prefix
        int number = atoi(token + 1);  // Convert the number part to int

        if (type == 'i') {
            pids[INPUT] = number;
        } else if (type == 'd') {
            pids[DRONE] = number;
        } else if (type == 'o') {
            pids[OBSTACLE] = number;
        } else if (type == 't') {
            pids[TARGET] = number;
        } else if (type == 'b') {
            pids[BLACKBOARD] = number;
        }else if (type == 'w') {
            pids[WATCHDOG] = number;
        }
        token = strtok(NULL, ",");
    }

     // Write the PID values to the output file
    for (int i = 0; i < 6; i++) {
        fprintf(file, "pid[%d] = %d\n", i, pids[i]);
        fflush(file);
    }

    char settings[50]; 


    // if (read(fds[INPUT][askrd], settings, 50) == -1){
    //     fprintf(file, "Error reading input\n");
    //     fflush(file);
    //     exit(EXIT_FAILURE);
    // }

    // if (write(fds[INPUT][recwr], ack, strlen(ack) + 1) == -1){
    //     fprintf(file, "Error sending ack\n");
    //     fflush(file);
    // }

    // token = strtok(settings, ";");
    // int diffic;
    // if (token != NULL) {
    //     // Copia il primo token (il nome) in name
    //     strncpy(name, token, sizeof(name));
    //     name[sizeof(name) - 1] = '\0'; // Assicurati che la stringa sia terminata correttamente

    //     // Prendi il prossimo token (il numero)
    //     token = strtok(NULL, ";");
    //     if (token != NULL) {
    //         // Converti il numero in intero
    //         diffic = atoi(token);
    //     } else {
    //         fprintf(file, "Errore: numero mancante.\n");
    //     }
    // } else {
    //     fprintf(file, "Errore: input non valido.\n");
    // }
    
    // if(diffic == 1){
    //     status.difficulty = 1;
    // } else if(diffic == 2){
    //     status.difficulty = 2;
    // }

    mapInit(file);
    elapsedTime = 0;


    while (1) {
        
        elapsedTime += PERIODBB/second;
        remainingTime = levelTime - (int)elapsedTime;

        if (remainingTime < 0){
            elapsedTime = 0;
            mvwprintw(map, nh/2, nw/2, "Time's up! Game over!");
            wrefresh(map);
            sleep(5);

            for(int j = 0; j < 6; j++){
                if (j != BLACKBOARD && pids[j] != 0){ 
                    if (kill(pids[j], SIGTERM) == -1) {
                    fprintf(file,"Process %d is not responding or has terminated\n", pids[j]);
                    fflush(file);
                    }

                    fprintf(file, "Killed process %d\n", pids[j]);
                    fflush(file);
                }
            }

            fprintf(file, "terminating blackboard\n");
            fflush(file);
            fclose(file);
            exit(EXIT_SUCCESS);
        }

        if(elapsedTime >= resetMap && diffic == HARD){
            resetMap += MAPRESET;
            fprintf(file, "Resetting map\n");
            fflush(file);
            createNewMap();
        }

        // if (targetsHit >= numTarget) {
        //     fprintf(file, "All targets reached\n");
        //     fflush(file);
        //     targetsHit = 0;
        //     collision = 0;
        //     for (int i = 0; i < MAX_TARGET; i++) {
        //        status.targets.value[i] = i + 1;
        //     }

        //     status.level++;            
        //     createNewMap();
        // }

        // Update the main window
        werase(win);
        werase(map);
        box(map, 0, 0);

        drawMenu(win);
        drawDrone(map);
        drawObstacle(map);
        drawTarget(map);
        wrefresh(win);
        wrefresh(map);
        
        //FDs setting for select
        FD_ZERO(&readfds);
        FD_SET(fds[DRONE][askrd], &readfds);
        //FD_SET(fds[INPUT][askrd], &readfds);
        //FD_SET(fds[OBSTACLE][askrd], &readfds);
        //FD_SET(fds[TARGET][askrd], &readfds); 

        

        int fdsQueue [4];
        int ready = 0;

        int sel = select(nfds, &readfds, NULL, NULL, &tv);
        
        if (sel == -1) {
            perror("Select error");
            break;
        } 

        if (FD_ISSET(fds[DRONE][askrd], &readfds)) {
            fdsQueue[ready] = fds[DRONE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[INPUT][askrd], &readfds)) {
            fdsQueue[ready] = fds[INPUT][askrd];
            ready++;
        }
        if (FD_ISSET(fds[OBSTACLE][askrd], &readfds)) {
            fdsQueue[ready] = fds[OBSTACLE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[TARGET][askrd], &readfds)) {
            fdsQueue[ready] = fds[TARGET][askrd];
            ready++;
        }

        if(ready > 0){
            unsigned int rand = randomSelect(ready);
            int selected = fdsQueue[rand];
            //detectCollision(&status.drone, &prevDrone, &status.targets, file);

            if (selected == fds[DRONE][askrd]){
                fprintf(file, "selected drone\n");
                fflush(file);
                
                readMsg(fds[DRONE][askrd], &msg, &status, 
                                "[BB] Error reading drone position", file);

                if(status.msg == 'R'){
                    if (collision)
                    {
                        collision = 0;

                        fprintf(file, "Send new map to [DRONE]\n");
                        fflush(file);
                        
                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", file);

                        readMsg(fds[DRONE][askrd], &msg, &status, 
                                "[BB] Error reading drone position", file);

                    }else{
                        
                        fprintf(file, "[BB] Asking drone position to [DRONE]\n");
                        fflush(file);
                        
                        status.msg = 'A';

                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", file);

                        status.msg = '\0';

                        readMsg(fds[DRONE][askrd], &msg, &status, 
                                "[BB] Error reading drone position", file);
                        

                        fprintf(file, "Drone updated position: %d,%d\n", status.drone.x, status.drone.y);
                        fflush(file);
                    }
                }
                  
            } else if (selected == fds[INPUT][askrd]){
                // fprintf(file, "selected input\n");
                // fflush(file);
                // char inp [12]; 
                
                // if (read(fds[INPUT][askrd], inp, 12) == -1){
                //     fprintf(file, "Error reading input\n");
                //     fflush(file);
                //     exit(EXIT_FAILURE);
                // }
                
                // fprintf(file, "Received: %s\n", inp);
                // fflush(file);

                // if(inp[2] == 'P'){
                //     fprintf(file, "Pause\n");
                //     fflush(file);
                //     mode = PAUSE;
                //     if (write(fds[INPUT][recwr], ack, strlen(ack) + 1) == -1){
                //     fprintf(file, "Error sending ack\n");
                //     fflush(file);
                //     }
                //     fprintf(file, "Sended ack\n");
                //     fflush(file);

                //     char pause[12];
                //     for(int i = 0; i < 12; i++){
                //         pause[i] = '\0';
                //     }  
                //     while(pause[2] != 'P' && pause[2] != 'q'){
                //         fprintf(file, "Waiting for play\n");
                //         fflush(file);
                //         if (read(fds[INPUT][askrd], pause, 12) == -1){
                //         fprintf(file, "Error reading input\n");
                //         fflush(file);
                //         exit(EXIT_FAILURE);
                //         }
                //     }
                //     if(pause[2] == 'P'){
                //         fprintf(file, "Play\n");
                //         fflush(file);
                //         mode = PLAY;
                //     }else if(pause[2] == 'q'){
                //         fprintf(file, "Quit\n");
                //         fflush(file);
                //         for(int j = 0; j < 6; j++){
                //             if (j != BLACKBOARD && pids[j] != 0){ 
                //                 if (kill(pids[j], SIGTERM) == -1) {
                //                 fprintf(file,"Process %d is not responding or has terminated\n", pids[j]);
                //                 fflush(file);
                //                 }

                //                 fprintf(file, "Killed process %d\n", pids[j]);
                //                 fflush(file);
                //             }
                //         }

                //         fprintf(file, "terminating blackboard\n");
                //         fflush(file);
                //         fclose(file);
                //         exit(EXIT_SUCCESS);
                //     }
                //     continue;

                // }else if (inp[2] == 'q'){
                //     for(int j = 0; j < 6; j++){
                //         if (j != BLACKBOARD && pids[j] != 0){ 
                //             if (kill(pids[j], SIGTERM) == -1) {
                //             fprintf(file,"Process %d is not responding or has terminated\n", pids[j]);
                //             fflush(file);
                //             }

                //             fprintf(file, "Killed process %d\n", pids[j]);
                //             fflush(file);
                //         }
                //     }

                //     fprintf(file, "terminating blackboard\n");
                //     fflush(file);
                //     fclose(file);
                //     exit(EXIT_SUCCESS);
                // }

                // char rec[2];
                // if(read(fds[DRONE][askrd], &rec, 2) == -1){
                //     fprintf(file, "Error reading input\n");
                //     fflush(file);
                //     exit(EXIT_FAILURE);
                // }
                // if(rec[0] == 'R'){
                //     if (write(fds[DRONE][recwr], inp, 12) == -1) {
                //         fprintf(file, "[BB] Error asking drone position\n");
                //         fflush(file);
                //         exit(EXIT_FAILURE);
                //     }    
                // }

                // fprintf(file, "drone ready\n"); 
                // fflush(file);

                // fprintf(file, "Sended ack and reading drone position\n");
                // fflush(file);

                // readMsg(fds[DRONE][askrd], &msg, &status, 
                //                 "[BB] Error reading drone position", file);
                                
                // if (write(fds[INPUT][recwr], droneInfo_str, strlen(droneInfo_str) + 1) == -1){
                //     fprintf(file, "Error sending ack\n");
                //     fflush(file);
                //     exit(EXIT_FAILURE);
                // }

            } else if (selected == fds[OBSTACLE][askrd] || selected == fds[TARGET][askrd]){
                fprintf(file, "selected obstacle or target\n");
                fflush(file);

                createNewMap();
            }else{
                fprintf(file, "Problems\n");
                fflush(file);
            }
        }
        usleep(PERIODBB);
    }

    return 0;
}