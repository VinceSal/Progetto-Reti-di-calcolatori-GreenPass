/*
SERVER G

Tale ServerG comunica con il ServerV,il ClientS e il CLientT
IL ServerG:
1)Riceve l'id della tessera sanitaria per il check di validità dal ClientS
2)Richiede generalità della tessera al ServerV
3)Si occupa di prendere e spedire il Report del ClientT
*/

#include "header.h"

//Function per l'estrazione della data odierna che verrá utilizzata con il GreenPass
void create_today_date(DATE *start_date) {
    time_t time_tick;
    time_tick = time(NULL);

    //Function di localtime che permette di convertire un formato data a intero
    struct tm *s_date = localtime(&time_tick);
    //Si parte con i mesi di tm da 0
    s_date->tm_mon += 1;
    //Si somma 1900 perchè si contano 123 anni, di conseguenza per arrivare al 2023 impostiamo 1900           
    s_date->tm_year += 1900;       

    //Si assegnano i valori ai parametri di ritorno
    start_date->day = s_date->tm_mday ;
    start_date->month = s_date->tm_mon;
    start_date->year = s_date->tm_year;
}

/*Function di comunicazione con il ClientS che invierá l'identificativo della tessera sanitaria
Il ServerG richiede il report al ServerV e dopo le procedure di verifica comunica il risultato al ClientS*/
char check_id(char ID[]) {
    int socket_fd;
    int package_size;
    int welcome_size;
    struct sockaddr_in server_addr;
    char buffer[MAX_SIZE];
    char report;
    char bit_communication;
    GREEN_PASS gp;
    DATE today_date;

    //Si imposti a 0 bit_communication per far capire al ServerV che sta comunicando con il ServerG
    bit_communication = '0';

    //Creazione del descrittore del socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    //Strutture
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1036);

    //Si converte indirizzo IP,avendo preso in input una stringa in formato dotted,ad un indirizzo di rete in network order
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    //Si effettua connessione con il server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() error");
        exit(1);
    }

    //Si invia bit 0 per comunicare al ServerV di collegarsi con il ServerG
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    bit_communication = '1';

    //Si invii adesso bit 1 per comunicare al ServerV che deve controllare la validitá del GreenPass
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Si invia l'identificativo della tessera sanitaria ricevuto dal ClientS al SeverV
    if (full_write(socket_fd, ID, ID_SIZE)) {
        perror("full_write() error");
        exit(1);
    }

    //Si riceve il report dal ServerV
    if (full_read(socket_fd, &report, sizeof(char)) < 0) {
        perror("full_read() error");
        exit(1);
    }
    
    //Si riceve l'esito della verifica dal ServerV e:se 0 è non valido,se 1 è valido
    if (report == '1') {        
        //Si estrae il GreenPass associato alla tessera sanitaria inviata dal ClientS
        if (full_read(socket_fd, &gp, sizeof(GREEN_PASS)) < 0) {
            perror("full_read() error\n");
            exit(1);
        }
        
        //Si chiuda la connessione dopo aver ottenuto il GP associato
        close(socket_fd);

        //Function per ricavare la data odierna
        create_today_date(&today_date);

        /*Si veda se il GreenPass è concretamente valido dopo averlo confrontato con la data odierna
        Effettuiamo quindi il controllo anche mendiante l'uso della data di fine del GreenPass
         */
        if (today_date.year > gp.end_date.year) report = '0';
        if (report == '1' && today_date.year > gp.end_date.year && today_date.month > gp.end_date.month) report = '0';
        if (report == '1' && today_date.day > gp.end_date.year && today_date.month > gp.end_date.month && today_date.day > gp.end_date.day) report = '0';
        //Si fa un controllo se è presente il GreenPass di un utente positivo(gp.report == 0)
        if (report == '1' && gp.report == '0') report = '0'; 
    }
    return report;
}

//Funzione per la gestione della comunicazione con l'utente
void get_id(int connect_fd) {
    char report;
    char buffer[MAX_SIZE];
    char  ID[ID_SIZE];
    int index;
    int package_size;
    int welcome_size;

    //Si stampa un messaggio di benvenuto da inviare al ClientS dopo la connessione con il ServerG
    snprintf(buffer, WELCOME_SIZE, "[Le diamo il benvenuto nell'App CampaniaGP]\n");
    buffer[WELCOME_SIZE - 1] = 0;
    //Si mandi il messaggio scritto in precedenza
    if(full_write(connect_fd, buffer, WELCOME_SIZE) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Si legge il codice fiscale inviato dal ClientS
    if(full_read(connect_fd, ID, ID_SIZE) < 0) {
        perror("full_read error");
        exit(1);
    }

    //Si notifica all'utente la corretta ricezione dei dati che aveva inviato
    snprintf(buffer, MEX_SIZE, "L'identificativo della tessera sanitaria è corretto... \n");
    buffer[MEX_SIZE - 1] = 0;
    if(full_write(connect_fd, buffer, MEX_SIZE) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Function che invia l'identificativo della tessera sanitaria al ServerV e ricveve il report dal ServerG
    report = check_id(ID);

    //Si invia il report di validità del green pass al ClientS,per verificarne anche l'esistenza di quest'ultimo
    if (report == '1') {
        strcpy(buffer, "Il GreenPass è valido! \n");
        if(full_write(connect_fd, buffer, CT_ACK) < 0) {
            perror("full_write() error");
            exit(1);
        }
    } else if (report == '0') {
        strcpy(buffer, "Il GreenPass non è valido! \n");
        if(full_write(connect_fd, buffer, CT_ACK) < 0) {
            perror("full_write() error");
            exit(1);
        }
    } else {
        strcpy(buffer, "Ops,l'id non esiste! \n");
        if(full_write(connect_fd, buffer, CT_ACK) < 0) {
            perror("full_write() error");
            exit(1);
        }
    }
    //Si chiuda la connessione dopo il check di validità del GreenPass
    close(connect_fd);
}

char dispatch_report(REPORT package) {
    int socket_fd;
    struct sockaddr_in server_addr;
    char bit_communication;
    char buffer[MAX_SIZE];
    char report;

    bit_communication = '0';

    //Creazione del descrittore del socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1036);

    //Si converte l'indirizzo IP dal formato dotted decimal a stringa di bit
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    //Si effettua la connessione con il server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() error");
        exit(1);
    }

    //Si invia un bit di valore 0 al ServerV per informarlo che sta comunicando con ServerG
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Si invia un bit di valore 0 al ServerV per informarlo che deve comunicare con quest'ultimo per modificare il report del GreenPass
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Si invia il package del ClientT al ServerV
    if (full_write(socket_fd, &package, sizeof(REPORT)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Riceve il report dal ServerG
    if (full_read(socket_fd, &report, sizeof(report)) < 0) {
        perror("full_read() error");
        exit(1);
    }
    //Si chiuda la connessione dopo aver ricevuto il report
    close(socket_fd);

    return report;
}

void get_report(int connect_fd) {
    REPORT package;
    char report;
    char buffer[MAX_SIZE];


    //Si leggano i dati del pacchetto REPORT inviato dal ClientT
    if (full_read(connect_fd, &package, sizeof(REPORT)) < 0) {
        perror("full_read() error");
        exit(1);
    }

    report = dispatch_report(package);

    /*
    Si manda il messaggio del Report:se report == 1,l'identificativo della tessera sanitaria
    non esiste, altrimenti l'operazione avrà avuto successo
    */
    if (report == '1') {
        strcpy(buffer, "Ops,l'id non esiste! \n");
        if(full_write(connect_fd, buffer, CT_ACK) < 0) {
            perror("full_write() error");
            exit(1);
        }
    } else {
        strcpy(buffer, "L'operazione ha avuto successo!");
        if(full_write(connect_fd, buffer, CT_ACK) < 0) {
            perror("full_write() error");
            exit(1);
        }
    }
}

int main() {
    int connect_fd, listen_fd;
    struct sockaddr_in serv_addr;
    char bit_communication;
    pid_t pid;
    

    signal(SIGINT,intercept); 
    //Creazione descrizione del socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1037);

    //Si assegna la porta al server
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind() error");
        exit(1);
    }

    //Si mette il socket in ascolto,in attesa di nuove connessioni
    if (listen(listen_fd, 1035) < 0) {
        perror("listen() error");
        exit(1);
    }

    while(1) {
    printf("Attendendo GreenPass da scansionare...\n");


        //Si accetta una nuova connessione
        if ((connect_fd = accept(listen_fd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept() error");
            exit(1);
        }

        //Si crea il figlio;
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }

        if (pid == 0) {
            close(listen_fd);

            /*
            Il ServerVerifica riceve un bit come primo messaggio, che può avere valore 0 o 1, siccome si hanno due connessioni differenti
            Quando riceve come bit 1 allora il figlio gestirà la connessione con il ClientT
            Quando riceve come bit 0 allora il figlio gestirà la connessione con il ClientS
            */
            if (full_read(connect_fd, &bit_communication, sizeof(char)) < 0) {
                perror("full_read() error");
                exit(1);
            }
            //Si ricevono informazioni dal ClientT
            if (bit_communication == '1') get_report(connect_fd);
            //Si ricevono informazioni dal ClientS   
            else if (bit_communication == '0') get_id(connect_fd);  
            else printf("Client NOT FOUND\n");

            close(connect_fd);
            exit(0);
        } else close(connect_fd);
    }
    exit(0);
}