/*
CLIENT S(APP CAMPANIAGP)

Il ClientS rappresenta nel mondo reale quella che potrebbe essere un app(esempio: e-covid), per controllare date e
validità del GreenPass.
IL ClientS interagisce con il ServerG
*/

#include "header.h"

int main() {
    int socket_fd;
    //Informazioni IP-Porta
    struct sockaddr_in server_addr;
    char bit_communication;
    char report;
    char buffer[MAX_SIZE], ID[ID_SIZE];
    
    //Si inizializza il bit a 0 per inviarlo al ServerG
    bit_communication = '0'; 

    //Creazione del descrittore del socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    //Strutture
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1037);

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

    //Si invia un bit di valore 0 al ServerG per informarlo che la comunicazione deve avvenire con il ClientS
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    //Riceve il benvenuto dal ServerG
    if (full_read(socket_fd, buffer, WELCOME_SIZE) < 0) {
        perror("full_read() error");
        exit(1);
    }
    printf("%s\n\n", buffer);

    //Inserimento identificativo tessera sanitaria
    while (1) {
        printf("Inserisca l'identificativo della tessera sanitaria(MAX 10 caratteri): ");
        if (fgets(ID, MAX_SIZE, stdin) == NULL) {
            perror("fgets() error");
            exit(1);
        }
        //Nel caso in cui il numero di caratteri inseriti da tastiera fosse diverso dai 10 prestabiliti, si stampi il messaggio
        if (strlen(ID) != ID_SIZE) printf("Il numero di caratteri della tessera sanitaria è errato,si prega di inserire esattamente 10 cifre!\n\n");
        else {

            //Sostituzione invio con terminatore
            ID[ID_SIZE - 1] = 0;
            break;
        }
    }

    //Si invia il numero della tessera sanitaria da convalidare al ServerG
    if (full_write(socket_fd, ID, ID_SIZE)) {
        perror("full_write() error");
        exit(1);
    }

    //Si riceve l'ack
    if (full_read(socket_fd, buffer, MEX_SIZE) < 0) {
        perror("full_read() error");
        exit(1);
    }
    printf("\n%s\n\n", buffer);

    printf("Convalida in corso, attendere...\n\n");

    //Si attendono 3 secondi per completare l'operazione di verifica
    sleep(3);
    
    //Si riceve l'esito della scansione del GreenPass dal ServerG
    if (full_read(socket_fd, buffer, CS_ACK) < 0) {
        perror("full_read() error");
        exit(1);
    }
    printf("%s\n", buffer);

    close(socket_fd);

    exit(0);
}