/*
UTENTE(CLIENT)

L'Utente rappresenta il Client(nel diagramma all'interno della traccia)
Quest'ultimo instaura una connessione con il CentroVaccinale
*/

#include "header.h"

//Function per la creazione del pacchetto da inviare al CentroVaccinale
//Si utilizza un buffer nel quale sono contenute le stringhe, contenenti le generalità, da far leggere poi al CentroVaccinale
VAX create_package() {
    char buffer[MAX_SIZE];
    VAX create_pack;

    //Utente inserisce il nome
    printf("Nome Utente: ");
    if (fgets(create_pack.name, MAX_SIZE, stdin) == NULL) {
        perror("fgets() error");
    }

    //Si va ad inserire il terminatore al posto dell'invio inserito dalla fgets,poichè questo veniva contato ed inserito come carattere nella stringa
    create_pack.name[strlen(create_pack.name) - 1] = 0;

    //Utente inserisce il nome
    printf("Cognome Utente: ");
    if (fgets(create_pack.surname, MAX_SIZE, stdin) == NULL) {
        perror("fgets() error");
    }
    //Si va ad inserire il terminatore al posto dell'invio inserito dalla fgets,poichè questo veniva contato ed inserito come carattere nella stringa
    create_pack.surname[strlen(create_pack.surname) - 1] = 0;

    //Si inserisca l'identificativo(codice) della tessera sanitaria
    while (1) {
        printf("Inserisca l'identificativo della tessera sanitaria(MAX 10 caratteri): ");
        if (fgets(create_pack.ID, MAX_SIZE, stdin) == NULL) {
            perror("fgets() error");
            exit(1);
        }
        //Nel caso in cui il numero di caratteri inseriti da tastiera fosse diverso dai 10 prestabiliti, si stampi il messaggio
        if (strlen(create_pack.ID) != ID_SIZE) printf("Il numero di caratteri della tessera sanitaria è errato,si prega di inserire esattamente 10 cifre!\n\n");
        else {
            //Si va ad inserire il terminatore al posto dell'invio inserito dalla fgets,poichè questo veniva contato ed inserito come carattere nella stringa
            create_pack.ID[ID_SIZE - 1] = 0;
           break;
        }
    }
    return create_pack;
}

int main(int argc, char **argv) {
    int socket_fd;
    int package_size, welcome_size;
    //Informazioni IP-Porta
    struct sockaddr_in server_addr;
    //Package da spedire
    VAX package;
    char buffer[MAX_SIZE];
    char **alias;
    char *address;
    //Struttura per utilizzare la gethostbyname
	struct hostent *data; 

    if (argc != 2) {
        //perror: Produce un messaggio sullo standard error che descrive l’ultimo errore avvenuto durante una System call o una funzione di libreria
        perror("usage: <host name>"); 
        exit(1);
    }

    //Creazione del descrittore del socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1035);

    //Si converte dal nome al dominio ad indirizzo IP
    if ((data = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname() error");
		exit(1);
    }
	alias = data -> h_addr_list;

    //inet_ntop converte un indirizzo in una stringa
    if ((address = (char *)inet_ntop(data -> h_addrtype, *alias, buffer, sizeof(buffer))) < 0) {
        perror("inet_ntop() error");
        exit(1);
    }

    //Si converte l’indirizzo IP, preso in input come stringa in formato dotted, in un indirizzo di rete in network order
    if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    //Si effettua la connessione con il server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() error");
        exit(1);
    }
    //FullRead per leggere quanti byte invia il CentroVaccinale
    if (full_read(socket_fd, &welcome_size, sizeof(int)) < 0) {
        perror("full_read() error");
        exit(1);
    }
    //Si riceve il benevenuto dal CentroVaccinale
    if (full_read(socket_fd, buffer, welcome_size) < 0) {
        perror("full_read() error");
        exit(1);
    }
    printf("%s\n", buffer);

    //Si crea il pacchetto da inviare al CentroVaccinale
    package = create_package();

    //Si invia il pacchetto richiesto al CentroVaccinale
    if (full_write(socket_fd, &package, sizeof(package)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Ricezione del messaggio
    if (full_read(socket_fd, buffer, MEX_SIZE) < 0) {
        perror("full_read() error");
        exit(1);
    }
    printf("%s\n\n", buffer);

    exit(0);
}