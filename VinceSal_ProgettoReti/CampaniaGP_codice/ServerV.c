/*
SERVER V

Tale ServerV comunica con il CentroVaccinale e il ServerG
IL ServerV:
1)Riceve generalità e GP dal CentroVaccinale
2)Invia le varie informazioni riguardanti l'Utente al ServerG
3)Gestisce le modifiche, riguardanti i report di validità del GP, passate tramite il ServerG
*/

#include "header.h"

//Funzione che invia un GP richiesto dal ServerG
void gp_dispatch(int connect_fd) {
    char report;
    char ID[ID_SIZE];
    int fd;
    GREEN_PASS gp;

    //Scrive in ID l'identificativo della tessera sanitaria ricevuta dal ServerG da connect_fd
    if (full_read(connect_fd, ID, ID_SIZE) < 0) {
        perror("full_read() error");
        exit(1);
    }
    //Si apre il file rinominato ID, cioé l'identificativo inviato dal ServerG
    fd = open(ID, O_RDONLY);

    /*
    Nel caso in cui l'identificativo della tessera sanitaria inviata dal ClientS non dovesse esistere, la variabile globale errno catturerà ciò
    inviando un report uguale ad 1 al ServerG, il quale aggiornerà il ClientS dell'inesistenza di quest'ultimo. 
    Nel caso in cui l'identificativo  della tessera sanitaria esistesse, errno invierà un report uguale a 0, che sta ad indicare che 
    l'operazione è avvenuta correttamente
    */

    //errno = 2 se la open é fallita non avendo trovato alcun numero di tessera sanitaria ("No such file or directory")
    if (errno == 2) { 
        printf("L'identificativo della tessera sanitaria non esiste,si prega di riprovare...\n");
        report = '2';

        //Invia tale report al ServerG
        if (full_write(connect_fd, &report, sizeof(char)) < 0) {
            perror("full_write() error");
            exit(1);
        }
    } else {
        //Si accede in maniera mutuamente esclusiva,cioè LOCK_EX definisce che solo un processo alla volta può accedere al blocco
        if (flock(fd, LOCK_EX) < 0) {
            perror("flock() error");
            exit(1);
        }
        //Lettura del GreenPass del file aperto in precedenza
        if (read(fd, &gp, sizeof(GREEN_PASS)) < 0) {
            perror("read() error");
            exit(1);
        }
        //Si sblocchi il lock,con LOCK_UN che rimuove la function flock con LOCK_EX definita in precedenza
        if(flock(fd, LOCK_UN) < 0) {
            perror("flock() error");
            exit(1);
        }
        //chiudiamo il file 
        close(fd);
        report = '1';

        //Si invia il report al ServerG
        if (full_write(connect_fd, &report, sizeof(char)) < 0) {
            perror("full_write() error");
            exit(1);
        }

        //Si manda il GreenPass richiesto al ServerG che controllerà la sua validità
        if(full_write(connect_fd, &gp, sizeof(GREEN_PASS)) < 0) {
            perror("full_write() error");
            exit(1);
        }
    }
}

//Function che modifica il report sotto richiesta del ClientT
void edit_report(int connect_fd) {
    REPORT package;
    GREEN_PASS gp;
    int fd;
    char report;

    //Riceve il pacchetto REPORT dal ServerG proveniente dal ClientT,contenente l'identificativo della tessera sanitaria e il referto del tampone
    if (full_read(connect_fd, &package, sizeof(REPORT)) < 0) {
        perror("full_read() error");
        exit(1);
    }

    //Si apre il file contenente il GreenPass relativo all'identificativo della tessera ricevuto dal ClientT
    fd = open(package.ID, O_RDWR);

    /*
    Nel caso in cui l'identificativo della tessera sanitaria inviata dal ClientT non dovesse esistere, la variabile globale errno catturerà ciò
    inviando un report uguale ad 1 al ServerG, il quale avvertirà il ClientT dell'inesistenza di quest'ultimo. 
    Nel caso in cui l'identificativo  della tessera sanitaria esistesse, errno invierà un report uguale a 0, che sta ad indicare che 
    l'operazione è avvenuta correttamente
    */    
    if (errno == 2) {
        printf("L'identificativo della tessera sanitaria non esiste,si prega di riprovare...\n");
        report = '1';
    } else {
        //Si accede in maniera mutuamente esclusiva 
        if(flock(fd, LOCK_EX | LOCK_NB) < 0) { //LOCK_NB ritorno immediato
            perror("flock() error");
            exit(1);
        }
        //Si legge il file aperto contenente il GreenPass relativo all'identificativo della tessera ricevuta dal ClientT
        if (read(fd, &gp, sizeof(GREEN_PASS)) < 0) {
            perror("read() error");
            exit(1);
        }

        //Si assegna al report del GreenPass associato all'ID del report che quest'ultimi voglion modificare
        gp.report = package.report;
        //Si ritorna all'inizio dello stream del file
        lseek(fd, 0, SEEK_SET);

        //Si va a sovrascrivere i campi del GreenPass nel file binario con nome il numero di tessera sanitaria del green pass
        if (write(fd, &gp, sizeof(GREEN_PASS)) < 0) {
            perror("write() error");
            exit(1);
        }
        //Sblocchiamo il lock
        if(flock(fd, LOCK_UN) < 0) {
            perror("flock() error");
            exit(1);
        }
        report = '0';
    }
    //Si invia il report al ServerG
    if (full_write(connect_fd, &report, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }
}


//funzione per la comunicazione con il ServerG
//Si ricava il GreenPass dal filesystem dalla tessera sanitaria ricevuta e lo invia al ServerG
void ServerG_comunication(int connect_fd) {
    char bit_communication;

    /*
    Dato che si utilizzano 2 funzioni diverse:
    se riceve 0 bit,ServerV modificherá report
    se riceve 1 bit,ServerV invierá il GreenPass
    */
    if (full_read(connect_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_read() error");
        exit(1);
    }
    if (bit_communication == '0') edit_report(connect_fd);
    else if (bit_communication == '1') gp_dispatch(connect_fd);
    else printf("bit_communication NOT FOUND\n\n");
}

//funzione per la comunicazione con il CentroVaccinale
void CentroV_comunication(int connect_fd) {
    int fd;
    GREEN_PASS gp;

    //Si ricava il green pass dal centro vaccinale
    if (full_read(connect_fd, &gp, sizeof(GREEN_PASS)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Quando viene generato un nuovo GreenPass risulterà valido di defualt
    gp.report = '1'; //1 GP valido, 0 non valido

    //Per ogni tessera sanitaria crea un file contenente i dati ricevuti
    if ((fd = open(gp.ID, O_WRONLY| O_CREAT | O_TRUNC)) < 0) {
        perror("open() error");
        exit(1);
    }
    //Si scriva nel file creato i campi di GreenPass associati a quella tessera sanitaria
    if (write(fd, &gp, sizeof(GREEN_PASS)) < 0) {
        perror("write() error");
        exit(1);
    }
    //chiudiamo il file creato 
    close(fd);
}

int main() {
    signal(SIGINT,intercept); //Intercetta il segnale CTRL-C
    int listen_fd;
    int connect_fd;
    int package_size;
    struct sockaddr_in serv_addr;
    char bit_communication;
    pid_t pid;
    //Creazione descrizione del socke
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY:accetta associazioni da qualsiasi indirizzo associato al server
    serv_addr.sin_port = htons(1036);

    //Assegnazione della porta al server
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind() error");
        exit(1);
    }

    //Mette il socket in ascolto in attesa di nuove connessioni
    if (listen(listen_fd, 1035) < 0) {
        perror("listen() error");
        exit(1);
    }

    while(1) {

    printf("In attesa di nuove connesioni...\n\n");

        //Accetta nuova connessione
        if ((connect_fd = accept(listen_fd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept() error");
            exit(1);
        }

        //Creazione figlio;
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }

        //Blocco eseguito dal figlio
        if (pid == 0) {
            close(listen_fd);

            /*Il ServerV riceve un bit come primo messaggio, che può avere valore 0 o 1, avendo due connessioni differenti:
              Quando riceve come bit 1,il figlio gestisce la connessione con il CentroVaccinale
              Quando riceve come bit 0,il figlio gestisce la connessione con il ServerG*/

            if (full_read(connect_fd, &bit_communication, sizeof(char)) < 0) {
                perror("full_read() error");
                exit(1);
            }
            if (bit_communication == '1') CentroV_comunication(connect_fd);
            else if (bit_communication == '0') ServerG_comunication(connect_fd);
            else printf("Client NOT FOUND\n\n");

            close(connect_fd);
            exit(0);
        } else close(connect_fd); //Blocco eseguito dal padre
    }
    exit(0);
}