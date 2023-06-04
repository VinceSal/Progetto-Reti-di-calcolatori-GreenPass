//Header File

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <errno.h>       //Libreria per la gestione degli errori
#include <fcntl.h>       //Libreria per il controllo dei file
#include <sys/socket.h>  //Libreria per le definizioni dei socket
#include <arpa/inet.h>   //Libreria contenente le definizioni per le operazioni Internet
#include <time.h>        //Libreria che fa accedere alle funzioni di acquisizione e manipolazione del tempo
#include <signal.h>      //Libreria per la gestione dei segnali
#include <netdb.h>       //Libreria contenente le definizioni per le operazioni del database di rete

#define MAX_SIZE 1024    //Si dichiara la dimensione massima che deve avere il buffer
#define ID_SIZE 11       //Si dichiara la dimensione del codice della tessera sanitaria
#define WELCOME_SIZE 108 //Si dichiara la dimensione del messaggio di benvenuto dal ServerG
#define MEX_SIZE 64      //Si dichiara la dimensione del messaggio ricevuto dal CentroVaccinale
#define CS_ACK 39        //Si dichiara la dimensione dell'ACK ricevuto dal ClientS
#define CT_ACK 39        //Si dichiara la dimensione degli ACK inviati al ClientT

/*----------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------*/

//Struct data(giorno,mese,anno)
typedef struct {
    int day;
    int month;
    int year;
} DATE;

//Struct del pacchetto inviato dal CentroVaccinale con id utente, data inizio gp, data fine gp
typedef struct {
    char ID[ID_SIZE];
    char report; 
    DATE start_date;
    DATE end_date;
} GREEN_PASS;

//Struct del pacchetto da mandare al CentroVaccinale con:nome,cognome ed identificativo della tessera sanitaria 
typedef struct {
    char name[MAX_SIZE];
    char surname[MAX_SIZE];
    char ID[ID_SIZE];
} VAX;

//Struct del pacchetto del ClientT contenente ID e referto di validitá
typedef struct  {
    char ID[ID_SIZE];
    char report;
} REPORT;

/*----------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------*/

//Function che cattura il segnale CTRL-C 
void intercept (int sign){
    if (sign == SIGINT) {
        printf("\nUscita in corso...\n");
        sleep(3); //attende 2 secondi prima della prossima operazione
        printf("La ringraziamo per aver utilizzato il nostro servizio!\n");
        exit(0);
    }
}

//Legge esattamente count byte s iterando opportunamente le letture. Legge anche se viene interrotta da una System Call
ssize_t full_read(int fd, void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_read;
    n_left = count;
    while (n_left > 0) {  //repeat finchè non ci sono left
        if ((n_read = read(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; // Se si verifica una System Call che interrompe ripete il ciclo
            else exit(n_read);
        } else if (n_read == 0) break; // Se sono finiti esce
        n_left -= n_read;
        buffer += n_read;
    }
    buffer = 0;
    return n_left;
}


//Scrive esattamente count byte s iterando opportunamente le scritture. Scrive anche se viene interrotta da una System Call
ssize_t full_write(int fd, const void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_written;
    n_left = count;
    while (n_left > 0) {  //repeat finchè non ci sono left
        if ((n_written = write(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; //Se si verifica una System Call che interrompe ripete il ciclo
            else exit(n_written); //Se non è una System Call,esce con un errore
        }
        n_left -= n_written;
        buffer += n_written;
    }
    buffer = 0;
    return n_left;
}