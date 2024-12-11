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

#define CHOOSENAME 0
#define CHOOSEBUTTON 1
#define CHOOSEDIFF 2

int nh, nw;
float scaleh = 1.0, scalew = 1.0;

int btnValues[9] ={0};
// char *default_text[9] = {"L-UP", "UP", "R-UP", "LEFT", "CENTER", "RIGHT", "L-DOWN", "DOWN", "R-DOWN"};
// char *menu_text[9] = {"W", "E", "R", "S", "D", "F", "X", "C", "V"};
char *droneInfoText[6] = {"Position x: ", "Position y: ", "Force x: ", "Force y: ", "Speed x ", "Speed y: "};
char *menuBtn[2] = {"Press P to pause", "Press Q to quit"};

int pid;
int fds[4]; 

char name[50] = ""; // Buffer per il testo
int level = 0;

float droneInfo[6] = {0.0};
char droneInfo_str[40];

int mode = MENU;   
int disp = CHOOSENAME;

WINDOW * winBut[9]; 
WINDOW * win;
WINDOW* control;

FILE *settingsfile;
FILE *file;

Drone_bb drone = {0, 0};
Force force = {0, 0};
Speed speed = {0, 0};

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

// void drawBtn(int b, int mode) {
//     char *text[9] = {};
//     for(int i = 0; i < 9; i++){
//         text[i] = default_text[i];
//     }
//     if(mode == MENU){
//         for(int i = 0; i < 9; i++){
//             text[i] = menu_text[i];
//         }
//     }

//     char btn[10] = "";
//     for (int i = 0; i < BUTTONS; i++) {
//         strcpy(btn, text[i]);
//         werase(winBut[i]);
//         // Calcolo della posizione centrata
//         int r = 2; // Riga centrale
//         int c = (BTNSIZEC - strlen(btn)) / 2; // Colonna centrata

//         // Stampa del testo centrato nel pulsante
//         if(i == b){
//           wattron(winBut[i], COLOR_PAIR(1));  
//         }
//         box(winBut[i], 0, 0); // Disegna il bordo del pulsante
//         mvwprintw(winBut[i], r, c, "%s", btn);
//         wrefresh(winBut[i]); // Aggiorna la finestra
//         if(i == b){
//           wattroff(winBut[i], COLOR_PAIR(1));  
//         }
//     }
// }

void drawBtn(int b) {
    char btn[10] = ""; // Stringa per ogni pulsante

    for (int i = 0; i < BUTTONS; i++) {
        // Ottieni il carattere dal valore ASCII
        if (btnValues[i] != 0) { // Verifica che il valore ASCII non sia vuoto
            snprintf(btn, sizeof(btn), "%c", (char)btnValues[i]); // Converte in carattere
        } else {
            snprintf(btn, sizeof(btn), " "); // Se vuoto, usa uno spazio
        }

        werase(winBut[i]);

        // Calcolo della posizione centrata
        int r = 2; // Riga centrale
        int c = (BTNSIZEC - strlen(btn)) / 2; // Colonna centrata

        // Stampa del testo centrato nel pulsante
        if (i == b) {
            wattron(winBut[i], COLOR_PAIR(1));  
        }
        box(winBut[i], 0, 0); // Disegna il bordo del pulsante
        mvwprintw(winBut[i], r, c, "%s", btn);
        wrefresh(winBut[i]); // Aggiorna la finestra
        if (i == b) {
            wattroff(winBut[i], COLOR_PAIR(1));  
        }
    }
}

void drawName(){
    const int prompt_row = nh / 2 - 2; // Riga leggermente sopra il centro
    const int name_row = nh / 2;     // Riga per il nome
    const char *prompt = "Choose a name:";

    werase(stdscr);
    box(stdscr, 0, 0);
    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, name_row, (nw - strlen(name)) / 2, "%s", name);
    wrefresh(stdscr);
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
        drawName();
    }

    // Mostra il nome inserito e termina
    werase(stdscr);
    box(stdscr, 0, 0);
    char confirmation[100];
    snprintf(confirmation, sizeof(confirmation), "Name set: %s", name);
    mvwprintw(stdscr, nh / 2, (nw - strlen(confirmation)) / 2, "%s", confirmation);
    wrefresh(stdscr);
}

void drawDifficulty() {
    werase(stdscr); // Pulisce la finestra
    box(stdscr, 0, 0); // Disegna il bordo della finestra

    // Testo delle righe
    const char *line1 = "Choose the difficulty you want to play";
    const char *line2 = "1 - easy: Target and obstacles are static";
    const char *line3 = "2 - hard: Target and obstacles are moving";

    // Calcolo delle posizioni verticali
    int y1 = nh / 2 - 1; // Posizione verticale per la prima riga
    int y2 = nh / 2;     // Posizione verticale per la seconda riga
    int y3 = nh / 2 + 1; // Posizione verticale per la terza riga

    // Calcolo delle posizioni orizzontali (centrato)
    int x1 = (nw - strlen(line1)) / 2;
    int x2 = (nw - strlen(line2)) / 2;
    int x3 = (nw - strlen(line3)) / 2;

    // Stampa le righe centrate
    mvwprintw(stdscr, y1, x1, "%s", line1);
    mvwprintw(stdscr, y2, x2, "%s", line2);
    mvwprintw(stdscr, y3, x3, "%s", line3);

    // Aggiorna la finestra
    wrefresh(stdscr);
}

void setDifficulty(){
    drawDifficulty();
    int ch = 0;
    while (ch != 49 && ch != 50) {
        ch = getch();
        if(ch == 49){
            level = 1;
            writeSecure("log.txt", "1", 2, 'o');
        }else if(ch == 50){
            level = 2;
            writeSecure("log.txt", "2", 2, 'o');
        }
        drawDifficulty();
        usleep(10000); 
    }
    werase(stdscr);
}

int keyAlreadyUsed(int key, int index ){
    for(int i = 0; i < index + 1; i++){
        if(btnValues[i] == key || key == MY_KEY_p || key == MY_KEY_q || key == MY_KEY_Q || key == MY_KEY_P){
            mvwprintw(stdscr, 13, 17, "%s", "Key already used");
            wrefresh(stdscr);
            return 1;
        }
    }
    return 0;
}
void setBtns(){
    werase(stdscr);
    box(stdscr, 0, 0);
    const char *line1 = "Do you want to use the default key configuration?";
    const char *line2 = "y - yes";
    const char *line3 = "n - no";

    // Calcola le posizioni orizzontali centrate
    int col1 = (nw - strlen(line1)) / 2;
    int col2 = (nw - strlen(line2)) / 2;
    int col3 = (nw - strlen(line3)) / 2;

    // Stampa le scritte centrate
    mvwprintw(stdscr, 10, col1, "%s", line1);
    mvwprintw(stdscr, 11, col2, "%s", line2);
    mvwprintw(stdscr, 12, col3, "%s", line3);

    btnSetUp(14,(col1 + col2)/2);

    wrefresh(stdscr); 

    drawBtn(99); //to make all the buttons white
    usleep(10000);
    int ch = getch();
    if (ch == MY_KEY_N || ch == MY_KEY_n) {
        for(int i = 0; i < BUTTONS; i++){
            werase(stdscr);
            box(stdscr, 0, 0);   
            mvwprintw(stdscr, 10, 10, "%s", "Choose wich key do you want to use for the highlighted direction?"); 
            wrefresh(stdscr);
            drawBtn(i);
            while ((ch = getch()) == ERR || keyAlreadyUsed(ch, i)) {
                //nessun tasto premuto dall'utente
                usleep(100000);
            }
            
            btnValues[i] = ch;
            usleep(100000);
        }
        werase(stdscr);
    }else if(ch == 121){
        return;
    }else{
        setBtns();
    }
}

void pauseMenu(){
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
}
 
void mainMenu(){
    mode = MENU;
    disp = CHOOSENAME;
    setName();
    disp = CHOOSEBUTTON;
    setBtns();
    disp = CHOOSEDIFF;
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
    if(rec[0] == 'A'){
        fprintf(file, "Ack received\n");
        fflush(file);
    }else{
        fprintf(file, "Error receiving ack\n");
        fflush(file);
    }
}

void drawInfo() {

    //Menu part1

    int initialrow = (int)((nh/3) - 6)/2 > 0 ? (int)((nh/3) - 6)/2 : 0;
    char droneInfoStr[6][20];

    for (int i = 0; i < 6; i++) {

        // Formatta il numero in una stringa
        if(i < 2){
            snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%d", (int)droneInfo[i]);
        }else{
            snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", droneInfo[i]);
        }
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

    int initialrow2 = ((nh / 3) + ((nh / 3 - 2) / 2) > 0) ? (( nh / 3) + ((nh / 3 - 2) / 2)) : (nh / 3);
    

    for (int i = 0; i < 2; i++) {

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
    if (mode == PLAY){
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
        drawBtn(99); //to make all the buttons white
        drawInfo();
    }else if( mode == MENU && disp == CHOOSENAME){
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

        drawName();
    }else if( mode == MENU && disp == CHOOSEBUTTON){
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

        btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
        drawBtn(99); //to make all the buttons white
    }else if (mode == MENU && disp == CHOOSEDIFF){
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

        drawDifficulty();
    }
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
    mode = PLAY;
    while (1) {

        int ch;

        droneInfo[0] = drone.x;
        droneInfo[1] = drone.y;
        droneInfo[2] = force.x;
        droneInfo[3] = force.y;
        droneInfo[4] = speed.x;
        droneInfo[5] = speed.y;

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
                drawBtn(99); //to make all the buttons white
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
                drawBtn(btn);
                usleep(100000);
                werase(win);
                box(win, 0, 0);       
                wrefresh(win);
                drawBtn(99); //to make all the buttons white

                fprintf(file,"msg: %s\n", msg);
                fflush(file);
                // Send the message to the blackboard
                char rec[2];
                if(write(fds[askwr],msg,strlen(msg) + 1) == -1){
                    fprintf(file, "[INPUT] Error sending message\n");
                    fflush(file);
                }
                if(read(fds[recrd], &droneInfo_str, sizeof(droneInfo_str)) == -1){
                    fprintf(file, "[INPUT] Error reading ack\n");
                    fflush(file);
                }

                fprintf(file,"Received: %s\n", droneInfo_str);
                fflush(file);

                sscanf(droneInfo_str, "%d;%d;%f;%f;%f;%f", 
                        &drone.x, &drone.y, 
                        &force.x, &force.y, 
                        &speed.x, &speed.y);
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
                pauseMenu();
                usleep(10000);
            }
            if(ch == MY_KEY_P || ch == MY_KEY_p){
                wrefresh(stdscr);
                mode = PLAY;
                strcat(msg, "P");
                fprintf(file,"sending play: %s\n",msg);
                fflush(file);
                write(fds[askwr],msg,strlen(msg) + 1);
            }else if(ch == MY_KEY_Q || ch == MY_KEY_q){
                wrefresh(stdscr);
                strcat(msg, "q");
                fprintf(file,"sending quit: %s\n",msg);
                fflush(file);
                write(fds[askwr],msg,strlen(msg) + 1);
                
            }
        }

    }
    return 0;
}