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

#define PERIODBB 10000  // [us]

int nh, nw;
float scaleh = 1.0, scalew = 1.0;
int pid;

char ack [2] = "A\0";

int fds[4][4] = {0};

WINDOW * win;

Drone_bb drone;
Targets targets;
Obstacles obstacles;

FILE *conffile;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(BLACKBOARD,100);
    }
}

char drone_str[6];
char target_str[len_str_targets];
char obstacles_str[len_str_obstacles];
char send_dronetarget_str[len_str_targets + 10];
char temp[len_str_targets + 2];
char send_targetobs_str[len_str_obstacles + len_str_targets + 2];

void resizeHandler(int sig){
    getmaxyx(stdscr, nh, nw);  /* get the new screen size */
    scaleh = ((float)nh / (float)WINDOW_LENGTH);
    scalew = (float)nw / (float)WINDOW_WIDTH;
    endwin();
    initscr();
    start_color();
    curs_set(0);
    noecho();
    win = newwin(nh, nw, 2, 0); 
}

void mapInit(FILE *file){

   
    // receiving drone position
    // fprintf(file, "Sending new map to [DRONE]: %s\n", send_targetobs_str);
    // fflush(file);

    if (read(fds[DRONE][askrd], &drone_str, sizeof(drone_str)) == -1){
        fprintf(file,"[BB] Error reading drone position\n");
        fflush(file);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Drone position: %s\n", drone_str);
    fflush(file);
    fromStringtoDrone(&drone, drone_str, file);


    // send drone position to target

    // fprintf(file, "Sending drone position to [TA] (%s)\n", drone_str);
    // fflush(file);
    if (write(fds[TARGET][recwr], &drone_str, sizeof("12;34")) == -1) {
        fprintf(file,"[BB] Error sending drone position to [TARGET]\n");
        fflush(file);
        exit(EXIT_FAILURE);
    }

    // receiving target position

    // fprintf(file, "Reading target from [TA] \n");
    // fflush(file);
    if (read(fds[TARGET][askrd], &target_str, sizeof(target_str)) == -1){
        fprintf(file,"[BB] Error reading target\n");
        fflush(file);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Target position %s\n", target_str);
    fflush(file);

    //fromStringtoPositions(&targets, target_str, file);
    concatenateStr(drone_str, target_str, send_dronetarget_str, sizeof(send_dronetarget_str), file);

    // send drone and target position to obstacle

    // fprintf(file, "Sending drone and target position to [OB]: %s\n", send_dronetarget_str);
    // fflush(file);
    if (write(fds[OBSTACLE][recwr], &send_dronetarget_str, sizeof(send_dronetarget_str)) == -1) {
        fprintf(file,"[BB] Error sending drone and target position to [OBSTACLE]\n");
        fflush(file);
        exit(EXIT_FAILURE);
    }

    // receiving obstacle position

    // fprintf(file, "Reading obstacles positions from [OB]\n");
    // fflush(file);

    if (read(fds[OBSTACLE][askrd], &obstacles_str, sizeof(obstacles_str)) == -1){
        fprintf(file,"[BB] Reading obstacles positions\n");
        fflush(file);
        exit(EXIT_FAILURE);
    }
    
    fprintf(file, "Obstacles positions from [OB]: %s\n", obstacles_str);
    fflush(file);
    //fromStringtoPositions(&obstacles, obstacles_str, file);

    // sending map to drone
    concatenateStr("M", target_str, temp, sizeof(temp), file);
    concatenateStr(temp, obstacles_str, send_targetobs_str, sizeof(send_targetobs_str), file);

    // fprintf(file, "Sending new map to [DRONE]: %s\n", send_targetobs_str);
    // fflush(file);

    if (write(fds[DRONE][recwr], &send_targetobs_str, sizeof(send_targetobs_str)) == -1) {
        fprintf(file, "[BB] Error sending updated map\n");
        fflush(file);
        exit(EXIT_FAILURE);
    }

    char targetObstacle[100] = {0};
    strcat(targetObstacle, target_str);
    strcat(targetObstacle, ";");
    strcat(targetObstacle, obstacles_str);
    fromStringtoPositionsWithTwoTargets(targets.x, targets.y, obstacles.x,obstacles.y,targetObstacle, file);

    fprintf(file, "New obstacles:%s\n", targetObstacle);
    fprintf(file, "New obstacle:\n");
    for(int i = 0; i < NUM_OBSTACLES; i++){
        fprintf(file, "x: %d, y: %d\n", obstacles.x[i], obstacles.y[i]);
    }
    fprintf(file, "New targets:\n");
    for(int i = 0; i < NUM_TARGET; i++){
        fprintf(file, "x: %d, y: %d\n", targets.x[i], targets.y[i]);
    }
    fflush(file);
    // fprintf(file, "Reading drone position from [DRONE]\n");
    // fflush(file);

    // if (read(fds[DRONE][askrd], &drone_str, sizeof(drone_str)) == -1){
    //     fprintf(file,"[BB] Error reading drone position\n");
    //     fflush(file);
    //     exit(EXIT_FAILURE);
    // }
    // // fprintf(file, "Drone position: %s\n", drone_str);
    // // fflush(file);
    fromStringtoDrone(&drone, drone_str, file);
}

void drawDrone(WINDOW * win){
    int row = (int)(drone.x * scaleh);
    int col = (int)(drone.y * scalew);
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
    for(int i = 0; i < NUM_OBSTACLES; i++){
        mvwprintw(win, (int)(obstacles.x[i]*scaleh), (int)(obstacles.y[i]*scalew), "0");
    }
    wattroff(win, COLOR_PAIR(2)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawTarget(WINDOW * win) {
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(3)); 
    for(int i = 0; i < NUM_TARGET; i++){
        char val_str[2];
        sprintf(val_str, "%d", i/*targets.value[i]*/); // Converte il valore in stringa
        mvwprintw(win, targets.x[i] * scaleh, targets.y[i] * scalew, "%s", val_str); // Usa un formato esplicito
    } 
    wattroff(win, COLOR_PAIR(3)); 
    wattroff(win, A_BOLD); // Disattiva il grassetto
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

int main(int argc, char *argv[]) {

    // Log file opening
    FILE *file = fopen("outputbb.txt", "w");
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

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, nw, 2, 0); 
    scaleh = (float)nh / (float)WINDOW_LENGTH;
    scalew = (float)nw / (float)WINDOW_WIDTH;


    // Definizione delle coppie di colori
    init_pair(1, COLOR_BLUE, COLOR_BLACK);     // Testo blu su sfondo nero
    init_pair(2, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero
    init_pair(3, COLOR_GREEN, COLOR_BLACK);    // Testo verde su sfondo nero

    char drone_str[80];
    char forceO_str[80];
    char forceT_str[80];
    
    mapInit(file);
    sleep(1);

    char nome [100] = "mattia e chiara";
    conffile = fopen("appsettings.json", "r");

    if (conffile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;//1
    }

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), conffile); 

    fclose(conffile);

    cJSON *json = cJSON_Parse(jsonBuffer);// parse the text to json object

    if (json == NULL)
    {
        perror("Error parsing the file");
        return EXIT_FAILURE;
    }

    conf_ptr gameConfig = malloc(sizeof(conf_ptr)); // allocate memory dinamically
    
    strcpy(gameConfig->difficulty, cJSON_GetObjectItemCaseSensitive(json, "Difficulty")->valuestring);
    strcpy(gameConfig->playerName, cJSON_GetObjectItemCaseSensitive(json, "PlayerName")->valuestring);
    gameConfig->startingLevel = cJSON_GetObjectItemCaseSensitive(json, "StartingLevel")->valueint;
    
    fprintf(file, "Player name: %s\n", gameConfig->playerName);
    fprintf(file, "Difficulty: %s\n", gameConfig->difficulty);
    fprintf(file, "Starting level: %d\n", gameConfig->startingLevel);
    fflush(file);

    cJSON_Delete(json);//clean
    
    free(gameConfig);//malloc --> clean //delete the memory dinamically allocated

    while (1) {

        // Update the main window
        werase(win);
        box(win, 0, 0);
        drawDrone(win);
        
        drawObstacle(win);
        drawTarget(win);
        wrefresh(win);
        
        //FDs setting for select
        FD_ZERO(&readfds);
        FD_SET(fds[DRONE][askrd], &readfds);
        FD_SET(fds[INPUT][askrd], &readfds);
        FD_SET(fds[OBSTACLE][askrd], &readfds);
        FD_SET(fds[TARGET][askrd], &readfds); 

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

            if (selected == fds[DRONE][askrd]){
                fprintf(file, "selected drone\n");
                fflush(file);

                char rec[2];
                read(fds[DRONE][askrd], &rec, 2);
                fprintf(file, "Received: %s\n", rec);
                fflush(file);
                if(rec[0] == 'R'){
                    fprintf(file, "Asking drone position to [DRONE]\n");
                    fflush(file);
                    if (write(fds[DRONE][recwr], "A", 2) == -1) {
                        fprintf(file, "[BB] Error asking drone position\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }
                    if (read(fds[DRONE][askrd], &drone_str, sizeof(drone_str)) == -1){
                        fprintf(file,"[BB] Error reading drone position\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }
                    
                    fromStringtoDrone(&drone, drone_str, file);
                   
                    fprintf(file, "Drone position: %s\n", drone_str);
                    fflush(file);
                    
                    fprintf(file,"Drone position: %d,%d\n", drone.x, drone.y);
                    fflush(file);
                }
                  
            } else if (selected == fds[INPUT][askrd]){
                fprintf(file, "selected input\n");
                fflush(file);
                char inp [12]; 
                
                if (read(fds[INPUT][askrd], inp, 12) == -1){
                    fprintf(file, "Error reading input\n");
                    fflush(file);
                    exit(EXIT_FAILURE);
                }
                
                fprintf(file, "Received: %s\n", inp);
                fflush(file);

                char rec[2];
                if(read(fds[DRONE][askrd], &rec, 2) == -1){
                    fprintf(file, "Error reading input\n");
                    fflush(file);
                    exit(EXIT_FAILURE);
                }
                if(rec[0] == 'R'){
                    if (write(fds[DRONE][recwr], inp, 12) == -1) {
                        fprintf(file, "[BB] Error asking drone position\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }    
                }

                fprintf(file, "drone ready\n"); 
                fflush(file);

                if (write(fds[INPUT][recwr], ack, strlen(ack) + 1) == -1){
                    fprintf(file, "Error sending ack\n");
                    fflush(file);
                    exit(EXIT_FAILURE);
                }

                fprintf(file, "Sended ack and reading drone position\n");
                fflush(file);

                if (read(fds[DRONE][askrd], &drone_str, strlen(drone_str)) == -1){
                    fprintf(file,"[BB] Error reading drone position\n");
                    fflush(file);
                    exit(EXIT_FAILURE);
                }
                fprintf(file, "Drone position: %s\n", drone_str);
                fflush(file);
                fromStringtoDrone(&drone, drone_str, file);    

            } else if (selected == fds[OBSTACLE][askrd] || selected == fds[TARGET][askrd]){
                fprintf(file, "selected obstacle or target\n");
                fflush(file);

                char rec[2];
                read(fds[TARGET][askrd], &rec, 2);
                fprintf(file, "Received: %s\n", rec);
                fflush(file);
                if(rec[0] == 'R'){

                    fprintf(file, "Sending drone position to [TARGET] %s\n", drone_str);
                    fflush(file);

                    if (write(fds[TARGET][recwr], drone_str, strlen(drone_str)) == -1) {
                        fprintf(file, "[BB] Error sending drone position to target\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }

                    fprintf(file, "waiting for new targets\n");
                    fflush(file);

                    if (read(fds[TARGET][askrd], &target_str, 32) == -1){
                        fprintf(file,"[BB] Error reading target position\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }
                    
                    concatenateStr(drone_str, target_str, send_dronetarget_str, sizeof(send_dronetarget_str), file);

                    fprintf(file, "Sending new targets to obstacle: %s\n", send_dronetarget_str);
                    fflush(file);
                    
                    // fprintf(file, "Sending drone and target position to [OB]: %s\n", send_dronetarget_str);
                    // fflush(file);

                    if (write(fds[OBSTACLE][recwr], &send_dronetarget_str, sizeof(send_dronetarget_str)) == -1) {
                        fprintf(file,"[BB] Error sending drone and target position to [OBSTACLE]\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }

                    // receiving obstacle position

                    fprintf(file, "Reading obstacles positions from [OB]\n");
                    fflush(file);

                    if (read(fds[OBSTACLE][askrd], &obstacles_str, sizeof(obstacles_str)) == -1){
                        fprintf(file,"[BB] Reading obstacles positions\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }

                    char targetObstacle[100] = {0};
                    strcat(targetObstacle, target_str);
                    strcat(targetObstacle, ";");
                    strcat(targetObstacle, obstacles_str);
                    fromStringtoPositionsWithTwoTargets(targets.x, targets.y, obstacles.x,obstacles.y,targetObstacle, file);

                    fprintf(file, "New obstacles:%s\n", targetObstacle);

                    fprintf(file, "New obstacle:\n");
                    for(int i = 0; i < NUM_OBSTACLES; i++){
                        fprintf(file, "x: %d y: %d\n", obstacles.x[i], obstacles.y[i]);
                    }
                    fprintf(file, "New targets:\n");
                    for(int i = 0; i < NUM_TARGET; i++){
                        fprintf(file, "x: %d y: %d\n", targets.x[i], targets.y[i]);
                    }
                    fflush(file);

                    char newMap [100];
                    newMap[0] = 'M';
                    strcat(newMap, targetObstacle);
                    fprintf(file, "New map:%s\n", newMap);
                    fflush(file);

                    char rec[2];
                read(fds[DRONE][askrd], &rec, 2);
                fprintf(file, "Received: %s\n", rec);
                fflush(file);
                if(rec[0] == 'R'){
                    fprintf(file, "Asking drone position to [DRONE]\n");
                    fflush(file);
                    if (write(fds[DRONE][recwr], newMap, sizeof(newMap)) == -1) {
                        fprintf(file, "[BB] Error asking drone position\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }
                    if (read(fds[DRONE][askrd], &drone_str, sizeof(drone_str)) == -1){
                        fprintf(file,"[BB] Error reading drone position\n");
                        fflush(file);
                        exit(EXIT_FAILURE);
                    }
                    
                    fromStringtoDrone(&drone, drone_str, file);
                   
                    fprintf(file, "Drone position: %s\n", drone_str);
                    fflush(file);
                    
                    fprintf(file,"Drone position: %d,%d\n", drone.x, drone.y);
                    fflush(file);
                }
            }
                //if pObst == 1 || pTarget == 1
                    //send drone position to target
                    //read the target
                    //send drone position and target to obstacle
                    //read obstacle
            }else{
                fprintf(file, "Problems\n");
                fflush(file);
            }
        }
        usleep(PERIODBB);
    }

    return 0;
}