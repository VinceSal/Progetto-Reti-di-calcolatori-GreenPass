/*
CENTRO VACCINALE

Il CentroVaccinale comunica con l'Utente(Client) e il ServerV
Il CentroVaccinale deve:
1)Calcolare data d'inizio e data di fine del GreenPass
2)Ricevere generalità date dall'Utente
3)Inviare il GreenPass al ServerV
*/

#include "header.h"

//Function che calcola la data di inizio di validitá del GreenPass, cioè quando viene emesso il certificato
void create_start_date(DATE *start_date) {
    time_t time_tick;
    time_tick = time(NULL);

    //Function di localtime che permette di convertire un formato data a intero
    struct tm *s_date = localtime(&time_tick);
    //Si parte con i mesi di tm da 0
    s_date->tm_mon += 1; 
    //Si somma 1900 perchè si contano 123 anni, di conseguenza per arrivare al 2023 impostiamo 1900            
    s_date->tm_year += 1900;       

    printf("Il GreenPass è stato emesso in data : %02d:%02d:%02d\n", s_date->tm_mday, s_date->tm_mon, s_date->tm_year);

    //Si assegnano i valori ai parametri di ritorno
    start_date->day = s_date->tm_mday ;
    start_date->month = s_date->tm_mon;
    start_date->year = s_date->tm_year;
}

//Function che definisce la scadenza del GreenPass
void create_end_date(DATE *end_date) {
    time_t time_tick;   //struttura per la gestione della data
    time_tick = time(NULL); //Utilizziamo l'ora attuale per poi assegnarla alla variabile

    //Function di localtime che permette di convertire un formato data a intero
    struct tm *t_date = localtime(&time_tick);
    //Si somma 7 perchè il conteggio dei mesi va da 0 ad 11 (quindi impostiamo 6 mesi alla scadenza)
    t_date->tm_mon += 7;
    //Si somma 1900 perchè si contano 123 anni, di conseguenza per arrivare al 2023 impostiamo 1900
    t_date->tm_year += 1900;       

    //Si effettua un controllo nel caso in cui il vaccino sia stato fatto nei mesi in questione, provocando un aumento dell'anno
    //luglio
    if (t_date -> tm_mon == 13){ //13 perchè si riferisce ad un improbabile 13simo mese, attribuibile
        t_date -> tm_mon = 1;    //al primo(1) mese dell'anno successivo
        t_date -> tm_year++;
    }

    //agosto
    if (t_date -> tm_mon == 14){ //Si esegue la stessa azione vista per il primo if, anche negl'if successivi
        t_date -> tm_mon = 2;
        t_date -> tm_year++;
    }

    //settembre
    if (t_date -> tm_mon == 15){
        t_date -> tm_mon = 3;
        t_date -> tm_year++;
    }

    //ottobre
    if (t_date -> tm_mon == 16){
        t_date -> tm_mon = 4;
        t_date -> tm_year++;
    }

    //novembre
    if (t_date -> tm_mon == 17){
        t_date -> tm_mon = 5;
        t_date -> tm_year++;
    }

    //dicembre
    if (t_date -> tm_mon == 18){
        t_date -> tm_mon = 6;
        t_date -> tm_year++;
    }

    printf("Il GreenPass scadrà in data : %02d:%02d:%02d\n", t_date->tm_mday, t_date->tm_mon, t_date->tm_year);

    //Si assegnano i valori definiti da tale funzione con i vari if nella struct:
    end_date->day = t_date->tm_mday ;
    end_date->month = t_date->tm_mon;
    end_date->year = t_date->tm_year;
}

//Function per inviare il GreenPass al ServerV
void gp_dispatch(GREEN_PASS gp) {
    int socket_fd;
    struct sockaddr_in server_addr;
    char bit_communication, buffer[MAX_SIZE];

    //inizializziamo il bit a 1 da inviare al ServerVaccinale
    bit_communication = '1'; 

    //Creazione descrizione del socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1036); //porta

    //Conversione formato dell'indirizzo IP 
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    //Si effettua la connessione con il server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() error");
        exit(1);
    }

    //Invia un bit di valore 1 al ServerV per informarlo che la comunicazione deve avvenire con il CentroVaccinale
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Si invia il green pass al ServerV
    if (full_write(socket_fd, &gp, sizeof(gp)) < 0) {
        perror("full_write() error");
        exit(1);
    }
    
    close(socket_fd);
}

//Function per la generazione del GreenPass dopo la connessione con l'utente
void answer_user(int connect_fd) {
    //Centri Vaccinali in Campania(scelti in modo randomico)
    char *hub_name[] = {"Afragola", "Napoli", "Casoria", "Ischia", "Frattamaggiore", "Pomigliano", "Caserta", "Salerno", "Barra", "Benevento"}; 
    char buffer[MAX_SIZE];
    int index; 
    int welcome_size;
    int package_size;
    VAX package;
    GREEN_PASS gp;

    //Si selezioni in modo randomico uno dei centri vaccinali sovracitati
    srand(time(NULL));
    index = rand() % 10;

    //Si stampa nel buffer il messaggio di benvenuto
    snprintf(buffer, MAX_SIZE, "[Benvenuto al centro vaccinale di %s]\n\n", hub_name[index]);
    //Si definisce la lunghezza del messaggio di benvenuto
    welcome_size = sizeof(buffer);
    //Inviamo i byte di scrittura del buffer
    if(full_write(connect_fd, &welcome_size, sizeof(int)) < 0) {
        perror("full_write() error");
        exit(1);
    }
    //Successivamente si passa al buffer il messaggio di benvenuto
    if(full_write(connect_fd, buffer, welcome_size) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Si ricevono le informazioni per il GreenPass dall'Utente
    if(full_read(connect_fd, &package, sizeof(VAX)) < 0) {
        perror("full_read() error");
        exit(1);
    }

    printf("\nDati ricevuti\n");
    printf("Nome: %s\n", package.name);
    printf("Cognome: %s\n", package.surname);
    printf("Identificativo Tessera Sanitaria: %s\n\n", package.ID);

    //Si notifica all'utente la corretta ricezione dei dati che aveva inviato in precedenza
    snprintf(buffer, MEX_SIZE, "I suoi dati sono stati correttamente registrati!\n");
    if(full_write(connect_fd, buffer, MEX_SIZE) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Copio l'identificativo della tessera sanitaria,inviato dall'utente nel green pass,da inviare al ServerV
    strcpy(gp.ID, package.ID);
    //Si ottiene la data di inizo validità del Green Pass
    create_start_date(&gp.start_date);
    //Crea la data di scadenza (6 mesi)
    create_end_date(&gp.end_date);

    close(connect_fd);
    gp_dispatch(gp);
}

int main(int argc, char const *argv[]) {
    int listen_fd; //socket per l'attesa del collegamento
    int connect_fd; //socket dell'effettivo collegamento
    VAX package;
    struct sockaddr_in serv_addr;
    pid_t pid;
    signal(SIGINT,intercept); //Intercetta il segnale CTRL-C
    //Creazione descrizione del socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1035);

    //Assegnazione della porta al server
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind() error");
        exit(1);
    }

    //Mette il socket in ascolto,in attesa di nuove connessioni
    if (listen(listen_fd, 1035) < 0) {
        perror("listen() error");
        exit(1);
    }

    while(1) {

    printf("In attesa di nuove richieste di vaccinazione...\n");

        //Si accetta una nuova connessione
        if ((connect_fd = accept(listen_fd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept() error");
            exit(1);
        }

        //Creazione del figlio
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }

        if (pid == 0) {
            close(listen_fd);

            //Riceve informazioni dall'utente
            answer_user(connect_fd);

            close(connect_fd);
            exit(0);
        } else close(connect_fd);
    }
    exit(0);
}
