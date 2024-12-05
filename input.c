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
#include <cjson/cJSON.h>

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

int nh, nw;
float scaleh = 1.0, scalew = 1.0;

int btnValues[9] ={0};
char *default_text[9] = {"L-UP", "UP", "R-UP", "LEFT", "CENTER", "RIGHT", "L-DOWN", "DOWN", "R-DOWN"};
char *menu_text[9] = {"Q", "W", "E", "A", "S", "D", "Z", "X", "C"};
char *droneInfoText[6] = {"Position x: ", "Position y: ", "Force x: ", "Force y: ", "Speed x ", "Speed y: "};
char *menuBtn[3] = {"Press P to pause", "Press L to save", "Press M to quit"};

int pid;

float droneInfo[6] = {0.0};

WINDOW * winBut[9]; 
WINDOW * win;
WINDOW* control;

FILE *settingsfile;
FILE *file;

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

void drawInfo() {

    //Menu part1

    int initialrow = (int)((nh/3) - 6)/2 > 0 ? (int)((nh/3) - 6)/2 : 0;
    char droneInfoStr[6][20];

    for (int i = 0; i < 6; i++) {

        // Formatta il numero in una stringa
        snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.2f", droneInfo[i]);

        // Calcola le lunghezze effettive delle stringhe
        int textLen = strlen(droneInfoText[i]);
        int valueLen = strlen(droneInfoStr[i]);

        // Larghezza totale della combinazione di testo e valore
        int totalLen = textLen + valueLen; // +1 per lo spazio tra le stringhe

        // Calcola la colonna per centrare l'intera combinazione
        int col = ((nw / 2) - totalLen) / 2;

        // fprintf(file,"Row: %d, Col: %d\n", initialrow + i, col);
        // fflush(file);

        // Stampa la riga centrata
        mvwprintw(control, initialrow + i, col, "%s%s", droneInfoText[i], droneInfoStr[i]);
        wrefresh(control);
    }

     // Menu part 2 

    int initialrow2 = ((nh / 3) + ((nh / 3 - 3) / 2) > 0) ? (( nh / 3) + ((nh / 3 - 3) / 2)) : (nh / 3);
    

    for (int i = 0; i < 3; i++) {

        // Calcola le lunghezze effettive delle stringhe
        int textLen = strlen(menuBtn[i]);

        // Calcola la colonna per centrare l'intera combinazione
        int col = ((nw / 2) - textLen) / 2;

        // fprintf(file,"Row: %d, Col: %d\n", initialrow + i, col);
        // fflush(file);

        // Stampa la riga centrata
        mvwprintw(control, initialrow2 + i, col, "%s", menuBtn[i]);
        wrefresh(control);
    }

    // Menu part 2 

    int initialrow3 = ((2 * nh / 3) + ((nh / 3 - 10) / 2) > 0) ? (( 2 * nh / 3) + ((nh / 3 - 10) / 2)) : (2 * nh / 3);
    

    for (int i = 0; i < 10; i++) {

        char playerInfo[100] = "Player: mohamad, score: 10, level: 1";

        // Calcola le lunghezze effettive delle stringhe
        int textLen = strlen(playerInfo);

        // Calcola la colonna per centrare l'intera combinazione
        int col = ((nw / 2) - textLen) / 2;

        // fprintf(file,"Row: %d, Col: %d\n", initialrow + i, col);
        // fflush(file);

        // Stampa la riga centrata
        mvwprintw(control, initialrow3 + i, col, "%s", playerInfo);
        wrefresh(control);
    }
    

}

void resizeHandler(int sig){
    getmaxyx(stdscr, nh, nw);  /* get the new screen size */
    scaleh = ((float)nh / (float)WINDOW_LENGTH);
    scalew = (float)nw / (float)WINDOW_WIDTH;
    endwin();
    initscr();
    start_color();
    curs_set(0);
    noecho();
    werase(win);
    werase(control);
    werase(stdscr);
    
    win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
    control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);
    box(win, 0, 0); 
    wrefresh(win);      
    box(control, 0, 0);
    wrefresh(control);
    btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
    drawBtn(99, DEFAULT); //to make all the buttons white
    drawInfo();    
}

void readConfig(){
    fprintf(file,"Reading configuration file\n");
    fflush(file);

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile); 
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer);// parse the text to json object

    if (json == NULL){
        perror("Error parsing the file");
    }
    conf_ptr gameConfig = malloc(sizeof(conf_ptr)); // allocate memory dinamically
    
    strcpy(gameConfig->difficulty, cJSON_GetObjectItemCaseSensitive(json, "Difficulty")->valuestring);
    strcpy(gameConfig->playerName, cJSON_GetObjectItemCaseSensitive(json, "PlayerName")->valuestring);
    gameConfig->startingLevel = cJSON_GetObjectItemCaseSensitive(json, "StartingLevel")->valueint;

    //for array
    cJSON *numbersArray = cJSON_GetObjectItemCaseSensitive(json, "DefaultBTN");//this is an array
    int arraySize = cJSON_GetArraySize(numbersArray);
    fprintf(file,"Array size: %d\n", arraySize);
    fflush(file);

    fprintf(file,"BtnValues:");
    fflush(file);

    for (int i = 0; i < arraySize; ++i) {
        cJSON *element = cJSON_GetArrayItem(numbersArray, i);
        if (cJSON_IsNumber(element)) {
            fprintf(file,"%d,",btnValues[i] = element->valueint);
            fflush(file);
        }
    }

    cJSON_Delete(json);//clean
    
    free(gameConfig);//malloc --> clean //delete the memory dinamically allocated

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    file = fopen("outputinput.txt", "a");
     
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;//1
    }

    readConfig();

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
    signal(SIGWINCH, resizeHandler);

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);

    init_pair(1, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero

    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, (int)(nw / 2), 0, 0); 
    control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

    //mainMenu();
    btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
 
    while (1) {
        int ch;
        if ((ch = getch()) == ERR) {
            //nessun tasto premuto dall'utente
            usleep(100000);
            werase(stdscr);
            wrefresh(stdscr);

            werase(win);
            werase(control);
            box(win, 0, 0);
            wrefresh(win); 
            box(control, 0 ,0);               
            wrefresh(control);   
            drawBtn(99, DEFAULT); //to make all the buttons white
            drawInfo();
        }else {
            //message to bb init
            char msg [12];

            for(int i = 0; i < 12; i++){
                msg[i] = '\0';
            }
            
            msg[0] = 'I';
            msg[1] = ';';

            //Refresh the auxiliary window
            

            int btn;
            
            // int ch = 99;
            if (ch == btnValues[0]) {
                btn = LEFTUP;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[1]) {
                btn = UP;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[2]) {
                btn = RIGHTUP;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[3]) {
                btn = LEFT;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[4]) {
                btn = CENTER;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[5]) {
                btn = RIGHT;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[6]) {
                btn = LEFTDOWN;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[7]) {
                btn = DOWN;
                strcat(msg, moves[btn]);
            } else if (ch == btnValues[8]) {
                btn = RIGHTDOWN;
                strcat(msg, moves[btn]);
            } else{
                btn = 99;   //Any of the direction buttons pressed
            } 

            werase(win);
            box(win, 0, 0);       
            wrefresh(win);
            drawBtn(btn, DEFAULT);
            usleep(100000);
            werase(win);
            box(win, 0, 0);       
            wrefresh(win);
            drawBtn(99, DEFAULT); //to make all the buttons white

            fprintf(file,"msg: %s\n", msg);
            fflush(file);
            // Send the message to the blackboard
            char rec[2];
            write(fds[askwr],msg,strlen(msg) + 1);
            read(fds[recrd], &rec, 2);
        }
        
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
