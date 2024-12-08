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
#include "keyboardMap.h"

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
char *menu_text[9] = {"W", "E", "R", "S", "D", "F", "X", "C", "V"};
char *droneInfoText[6] = {"Position x: ", "Position y: ", "Force x: ", "Force y: ", "Speed x ", "Speed y: "};
char *menuBtn[3] = {"Press P to pause", "Press Q to quit", "Press L to save"};

int pid;
int fds[4]; 

char name[50] = ""; // Buffer per il testo
int level = 0;

float droneInfo[6] = {0.0};

int mode = PLAY;   

WINDOW * winBut[9]; 
WINDOW * win;
WINDOW* control;

FILE *settingsfile;
FILE *file;

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(INPUT, file);
    }else if(signo == SIGTERM){
        fprintf(file, "Input is quitting\n");
        fflush(file);   
        fclose(file);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
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

void setName() {
    werase(stdscr);
    box(stdscr, 0, 0);

    const char *prompt = "Choose a name:";
    const int prompt_row = nh / 2 - 2; // Riga leggermente sopra il centro
    const int name_row = nh / 2;     // Riga per il nome
    const int error_row = nh / 2 + 2; // Riga per i messaggi di errore

    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    wrefresh(stdscr);

    int ch = 0;
    int pos = 0;

    while (ch != MY_KEY_ENTER) {
        ch = getch(); // Legge il prossimo carattere premuto

        if (ch == MY_KEY_BACK) {
            // Backspace: rimuove l'ultimo carattere se possibile
            if (pos > 0) {
                pos--;
                name[pos] = '\0'; // Termina correttamente la stringa
            }
        } else if (ch >= 32 && ch <= 126) { // Solo caratteri stampabili
            if (pos < sizeof(name) - 1) {
                name[pos++] = ch;
                name[pos] = '\0';
            }
        } else {
            // Mostra messaggio di errore per tasto non valido
            mvwprintw(stdscr, error_row, (nw - strlen("Invalid key, try again.")) / 2, "%s", "Invalid key, try again.");
        }

        // Ridisegna lo schermo
        werase(stdscr);
        box(stdscr, 0, 0);
        mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
        mvwprintw(stdscr, name_row, (nw - strlen(name)) / 2, "%s", name);
        wrefresh(stdscr);
    }

    // Mostra il nome inserito e termina
    werase(stdscr);
    box(stdscr, 0, 0);
    char confirmation[100];
    snprintf(confirmation, sizeof(confirmation), "Name set: %s", name);
    mvwprintw(stdscr, nh / 2, (nw - strlen(confirmation)) / 2, "%s", confirmation);
    wrefresh(stdscr);
}

void setDifficulty(){
    werase(stdscr);
    box(stdscr, 0, 0);
    mvwprintw(stdscr, 10, 17, "%s", "Choose the difficulty you want to play"); 
    mvwprintw(stdscr, 11, 17, "%s", "1 - easy"); 
    mvwprintw(stdscr, 12, 17, "%s", "2 - medium"); 
    mvwprintw(stdscr, 13, 17, "%s", "3 - hard"); 
    wrefresh(stdscr);
    int ch = 0;
    while (ch != 49 && ch != 50 && ch != 51) {
        ch = getch();
        if(ch == 49){
            level = 1;
        }else if(ch == 50){
            level = 2;
        }else if(ch == 51){
            level = 3;
        }
    }
    werase(stdscr);
    box(stdscr, 0, 0);
    char confirmation[100];
    snprintf(confirmation, sizeof(confirmation), "Difficulty set: %d", level);
    mvwprintw(stdscr, nh / 2, (nw - strlen(confirmation)) / 2, "%s", confirmation);
    wrefresh(stdscr);
}
void setBtns(){
    btnSetUp(25,27);
    werase(stdscr);
    box(stdscr, 0, 0);
    mvwprintw(stdscr, 10, 17, "%s", "Do you want to use the default key configuration?"); 
    mvwprintw(stdscr, 11, 17, "%s", "y - yes"); 
    mvwprintw(stdscr, 12, 17, "%s", "n - no"); 
    wrefresh(stdscr);
    drawBtn(99, MENU); //to make all the buttons white
    usleep(10000);
    int ch = getch();
    if (ch == 110){
        for(int i = 0; i < BUTTONS; i++){
            werase(stdscr);
            box(stdscr, 0, 0);   
            mvwprintw(stdscr, 10, 10, "%s", "Choose wich key do you want to use for the highlighted direction?"); 
            wrefresh(stdscr);
            drawBtn(i, DEFAULT);
            while ((ch = getch()) == ERR) {
                //nessun tasto premuto dall'utente
                usleep(100000);
            }
            if(ch == MY_KEY_p || ch == MY_KEY_q){
                mvwprintw(stdscr, 10, 20, "%s", "Key already used");
                usleep(100000);
                continue;
            } else{
                btnValues[i] = ch;
            }
            usleep(100000);
        }
        werase(stdscr);
    }else if(ch == 121){
        return;
    }else{
        setBtns();
    }
}
void mainMenu(){
    
    setName();
    setBtns();
    setDifficulty();
    
    werase(stdscr);
    wrefresh(stdscr);

    char settings[50] = {0};
    strcat(settings,name);
    char levelChoose [4];
    snprintf(levelChoose, sizeof(levelChoose), ";%d", level);
    strcat(settings, levelChoose);
    fprintf(file,"\nSettings: %s\n", settings);
    fflush(file);

    if(write(fds[askwr],settings,strlen(settings) + 1) == -1){
        fprintf(file, "Error sending settings\n");
        fflush(file);     
    }

    char rec[2];
    if(read(fds[recrd], &rec, 2) == -1){
        fprintf(file, "Error reading ack");
        fflush(file);
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
    
    wrefresh(stdscr);
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
    signal(SIGTERM, sig_handler);

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

    mainMenu();
    btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
 
    while (1) {
        int ch;
        if(mode == PLAY){
            fprintf(file,"Mode: PLAY\n");
            fflush(file);
            if ((ch = getch()) == ERR) {
                //nessun tasto premuto dall'utente
                usleep(100000);

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
                }else if (ch == MY_KEY_p || ch == MY_KEY_P){
                    btn = 112; //Pause
                    strcat(msg, "P");
                    mode = PAUSE;
                } else if (ch == MY_KEY_q || ch == MY_KEY_Q){
                    btn = 109; //Quit
                    strcat(msg, "q");
                    fprintf(file,"sending quit: %s\n",msg);
                    fflush(file);
                }else{
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
        }else if(mode == PAUSE){
            char msg [12];

            for(int i = 0; i < 12; i++){
                msg[i] = '\0';
            }
            
            msg[0] = 'I';
            msg[1] = ';';

            fprintf(file,"Mode: PAUSE\n");
            fflush(file);

            while ((ch = getch()) != MY_KEY_P && ch != MY_KEY_p && ch != MY_KEY_Q && ch != MY_KEY_q) {
                werase(stdscr);
                box(stdscr, 0, 0);
                const char *prompt = "Press P to play";
                const char *prompt2 = "Press Q to save & quit";
                int prompt_row = nh / 2 - 2; // Riga leggermente sopra il centro
                int name_row = nh / 2;     // Riga per il nome
                int error_row = nh / 2 + 2; // Riga per i messaggi di errore

                mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
                mvwprintw(stdscr, prompt_row + 1, (nw - strlen(prompt)) / 2, "%s", prompt2);

                wrefresh(stdscr);

                usleep(10000);
            }
            if(ch == MY_KEY_P || ch == MY_KEY_p){
                mode = PLAY;
                strcat(msg, "P");
                fprintf(file,"sending play: %s\n",msg);
                fflush(file);
                write(fds[askwr],msg,strlen(msg) + 1);
            }else if(ch == MY_KEY_Q || ch == MY_KEY_q){
                strcat(msg, "q");
                fprintf(file,"sending quit: %s\n",msg);
                fflush(file);
                write(fds[askwr],msg,strlen(msg) + 1);
                
            }
        }

    }
    return 0;
}