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
#include <signal.h>

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define HEIGHT  50
#define WIDTH   80

#define BUTTONS     9   
#define BTNSIZEC    10
#define BTNSIZER    5
#define BTNDISTC    11
#define BTNDISTR    5
#define BTNPOSC    20
#define BTNPOSR    20

#define LEFTUP    0
#define UP        1
#define RIGHTUP   2
#define LEFT      3
#define CENTER    4
#define RIGHT     5
#define LEFTDOWN  6
#define DOWN      7
#define RIGHTDOWN 8

#define DEFAULT 0
#define MENU 1
int btnValues[9] =   {113, 119, 101,
                97, 115, 100,
                122, 120, 99};

char *default_text[9] = {"L-UP", "UP", "R-UP", "LEFT", "CENTER", "RIGHT", "L-DOWN", "DOWN", "R-DOWN"};
char *menu_text[9] = {"Q", "W", "E", "A", "S", "D", "Z", "X", "C"};

int pid;

WINDOW * winBut[9]; 
WINDOW * win;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(INPUT,100);
    }
}

void btnSetUp (int row, int col){

    for(int i = 0; i < BUTTONS; i++){
        int newRow = row + ((i/3)*BTNDISTR);
        int newCol = col + ((i%3)*BTNDISTC);
        winBut[i] = newwin(BTNSIZER, BTNSIZEC, newRow, newCol);
    }
}

void drawBtn(int b, int mode) {
    char *text[9] = {};
    for(int i = 0; i < 9; i++){
        text[i] = default_text[i];
    }
    if(mode == MENU){
        for(int i = 0; i < 9; i++){
        text[i] = menu_text[i];
        }
    }

    char btn[10] = "";
    for (int i = 0; i < BUTTONS; i++) {
        strcpy(btn, text[i]);
        werase(winBut[i]);
        // Calcolo della posizione centrata
        int r = 2; // Riga centrale
        int c = (BTNSIZEC - strlen(btn)) / 2; // Colonna centrata

        // Stampa del testo centrato nel pulsante
        if(i == b){
          wattron(winBut[i], COLOR_PAIR(1));  
        }
        box(winBut[i], 0, 0); // Disegna il bordo del pulsante
        mvwprintw(winBut[i], r, c, "%s", btn);
        wrefresh(winBut[i]); // Aggiorna la finestra
        if(i == b){
          wattroff(winBut[i], COLOR_PAIR(1));  
        }
    }
}

void mainMenu(){
    btnSetUp(25,27);
    werase(win);
    box(win, 0, 0);   
    mvwprintw(win, 10, 17, "%s", "Do you want to use the default key configuration?"); 
    mvwprintw(win, 11, 17, "%s", "y - yes"); 
    mvwprintw(win, 12, 17, "%s", "n - no"); 
    wrefresh(win);
    drawBtn(99, MENU); //to make all the buttons white
    sleep(5);
    int ch = getch();
    if (ch == 110){
        for(int i = 0; i < BUTTONS; i++){
            werase(win);
            box(win, 0, 0);   
            mvwprintw(win, 10, 10, "%s", "Choose wich key do you want to use for the highlighted direction?"); 
            wrefresh(win);
            drawBtn(i, DEFAULT);
            int ch = getch();
            btnValues[i] = ch;
            usleep(100000);
        }
    }else if(ch == 121){
        return;
    }else{
        mainMenu();
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    FILE *file = fopen("outputinput.txt", "a");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    // FDs reading
    char *fd_str = argv[1];
    int fds[4]; 
    int index = 0;
    
    char *token = strtok(fd_str, ",");
    token = strtok(NULL, ","); 

    // FDs extraction
    while (token != NULL && index < 4) {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "i%d,", pid);

    if(writeSecure("log.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    init_pair(1, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero

    win = newwin(HEIGHT, WIDTH, 5, 5);

    //mainMenu();

    btnSetUp(BTNPOSR,BTNPOSC);
 
    while (1) {

        werase(win);
        box(win, 0, 0);       
        wrefresh(win);
        drawBtn(99, DEFAULT); //to make all the buttons white

        int btn;
        int ch = getch();
        // int ch = 99;
        if (ch == btnValues[0]) {
            btn = LEFTUP;
        } else if (ch == btnValues[1]) {
            btn = UP;
        } else if (ch == btnValues[2]) {
            btn = RIGHTUP;
        } else if (ch == btnValues[3]) {
            btn = LEFT;
        } else if (ch == btnValues[4]) {
            btn = CENTER;
        } else if (ch == btnValues[5]) {
            btn = RIGHT;
        } else if (ch == btnValues[6]) {
            btn = LEFTDOWN;
        } else if (ch == btnValues[7]) {
            btn = DOWN;
        } else if (ch == btnValues[8]) {
            btn = RIGHTDOWN;
        } else{
            btn = 99;   //Any of the direction buttons pressed
        } 

        drawBtn(btn, DEFAULT);
        usleep(100000);
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
