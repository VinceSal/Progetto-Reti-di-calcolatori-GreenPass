/*
CLIENT T(CAMPANIA GREENPASS)

Il ClientT rappresenta nel mondo reale quello che potrebbe essere ad esempio una società privata(o magari un ente
comunale), che si occupa del ripristino o l'invalidazione del GreenPass. 
IL ClientT interagisce con il ServerG
*/

#include "header.h"

int main(int argc, char **argv) {
    int socket_fd;
    //Informazioni IP-Porta
    struct sockaddr_in server_addr;
    REPORT package;
    char bit_communication;
    char buffer[MAX_SIZE];

    //Si inizializzi il bit a 1 da inviare al ServerG
    bit_communication = '1'; 

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

    //Si invia un bit di valore 1 al ServerG per informarlo che la comunicazione deve avvenire con il ClientT
    if (full_write(socket_fd, &bit_communication, sizeof(char)) < 0) {
        perror("full_write() error");
        exit(1);
    }

    printf("[Campania GreenPass]\n\n");
    //Si inserisce l'identificativo 
    while (1) {
        printf("Inserisca l'identificativo della tessera sanitaria(MAX 10 caratteri): ");
        if (fgets(package.ID, MAX_SIZE, stdin) == NULL) {
            perror("fgets() error");
            exit(1);
        }
        //Controllo sull'input dell'utente
        if (strlen(package.ID) != ID_SIZE) printf("Il numero di caratteri della tessera sanitaria è errato,si prega di inserire esattamente 10 cifre!\n\n");
        else {

            //Sostituzione invio con terminatore
            package.ID[ID_SIZE - 1] = 0;
            break;
        }
    }

    while (1) {
        printf("Inserire: 0 [GreenPass Non Valido]\n");
        printf("Inserire 1 [GreenPass Valido]\n");
        printf("[INSERISCI 0 o 1]: ");
        scanf("%c", &package.report);

        //Controllo sull'input dell'utente,nel caso in cui fosse diverso da 0 o da 1 si dovrà ripetere l'operazione
        if (package.report == '1' || package.report == '0') break;
        printf("Errore: valore inserito non valido,si prega di ritentare...\n\n");
    }

    //Controllo sull'input dell'utente
    if (package.report == '1') printf("\nInviando la richiesta di ripristino del GreenPass...\n");
    else printf("\nInviando la richiesta di sospensione del GreenPass...\n");

    //Si invia il pacchetto report al ServerG
    if (full_write(socket_fd, &package, sizeof(REPORT)) < 0) {
        perror("full_write() error");
        exit(1);
    }
    //Si riceve il messaggio di report dal ServerG
    if (full_read(socket_fd, buffer, CT_ACK) < 0) {
        perror("full_read() error");
        exit(1);
    }
    //Si attendano 3 secondi per simulare un caricamento 
    sleep(3);
    //Si stampa di seguito il messaggio del report ricevuto dal serverG
    printf("%s\n", buffer);

    exit(0);
}