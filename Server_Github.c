//per avviare il db da cmd bisogna fare:
//gcc -o nome_eseguibile sorgente.c -l mysqlclient

#define PORT 50000
#define BUFFER_SIZE 1024

#define MAX_QUERY_LEN 500
#define MAX_QUEUE_LEN 100
#define MAX_NAME_LEN 100
#define MAX_SURNAME_LEN 100
#define MAX_USERNAME_LEN 100
#define MAX_PASSWORD_LEN 100

#define DB_NAME "CAALMA.db"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <sqlite3.h>

void my_recv(int connected_fd, char *buffer) {
    bzero(buffer, BUFFER_SIZE);

    printf("\tReceiving...\n"); fflush(stdout);

    int bytes_recived = read(connected_fd, buffer, BUFFER_SIZE);
    //buffer[bytes_recived-1] = '\0';

    printf("\tReceived %s\n", buffer); fflush(stdout);
}

void my_send(int connected_fd, char *buffer) {
    printf("\tSending %s\n", buffer); fflush(stdout);

    int len = strlen(buffer);
    //buffer[bytes_recived-1] = '\n'; buffer[bytes_recived] = '\0';
    //buffer[len] = '\n'; buffer[len+1] = '\0';
    write(connected_fd, buffer, strlen(buffer));

    printf("\tSended %s\n", buffer); fflush(stdout);
}

void err_sys(const char* x) { 
    perror(x); 
    exit(1); 
}

void db_error(sqlite3 *db) {
    fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
}

int login(char *username, char *password) {
    sqlite3 *db;
    sqlite3_stmt *result;

    char *query = malloc(sizeof(char) * MAX_QUERY_LEN);
    sprintf(query, "SELECT COUNT(*) FROM utente WHERE username = '%s' AND password = '%s';", username, password);

    if ( sqlite3_open(DB_NAME, &db) != SQLITE_OK ) db_error(db);

    if ( sqlite3_prepare_v2(db, query, -1, &result, 0) != SQLITE_OK ) db_error(db);

    if ( sqlite3_step(result) != SQLITE_ROW ) db_error(db);

    int ret = (int)sqlite3_column_int(result, 0);

    sqlite3_finalize(result);
    sqlite3_close(db);

    return ret;
}

int check_signup(char* username){
    sqlite3 *db;
    sqlite3_stmt *result;

    char *query = malloc(sizeof(char) * MAX_QUERY_LEN);
    sprintf(query1, "SELECT COUNT(username) FROM utente WHERE utente='%s';", username);

    int ret = (int)sqlite3_column_int(result, 0);

    sqlite3_finalize(result);
    sqlite3_close(db);
    return ret;

}

//nella registrazione andiamo a mettere anche il nome e il cognome
//Poi quando si farà la registrazione

int signup(char *nome, char *cognome, char* username, char *password){
    sqlite3 *db;
    sqlite3_stmt *result;
    int flag= check_signup(username);
    if (flag == 0){
        char *query = malloc(sizeof(char) * MAX_QUERY_LEN);
        sprintf(query, "INSERT INTO utente VALUES ('%s', '%s', '%s', '%s');", nome, cognome, username, password);

        char *err_msg = NULL;

        if ( sqlite3_open(DB_NAME, &db) != SQLITE_OK ) db_error(db);

        if ( sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK ) {
            sqlite3_free(err_msg);
            db_error(db);
        }

        ret = sqlite3_last_insert_rowid(db);

        //sqlite3_finalize(result);
        sqlite3_close(db);
        return ret;
    }
    return -1;
}

void server(int connected_fd, struct sockaddr peer, unsigned int addr_len) {
    char buffer[BUFFER_SIZE];

    char nome[MAX_NAME_LEN];
    char cognome[MAX_SURNAME_LEN];
	char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    
     printf("\n --- Il server sta servendo --- \n"); fflush(stdout);

    int bytes_recived;

    my_recv(connected_fd, buffer);
    my_send(connected_fd, buffer);

	//dipende dalla stringa che mi passa carmine
    if ( strcmp(buffer, "login\n") == 0 ) {
        printf("\tEseguendo il login\n"); fflush(stdout);

        my_recv(connected_fd, buffer);
        strcpy(username, buffer);
        username[strlen(username)-1] = '\0';
        my_send(connected_fd, "username ok\n");

        my_recv(connected_fd, buffer);
        strcpy(password, buffer);
        password[strlen(password)-1] = '\0';
        my_send(connected_fd, "password ok\n");

        if ( login(username, password) ) {
            my_recv(connected_fd, buffer);
            my_send(connected_fd, "login ok");
        } else {
            my_recv(connected_fd, buffer);
            my_send(connected_fd, "login not ok");
        }
    }

	//dipende dalla stringa che mi passa carmine
    if ( strcmp(buffer, "signup\n") == 0 ) {
        printf("\tEseguedo l'iscrizione\n"); fflush(stdout);

        my_recv(connected_fd, buffer);
        strcpy(nome, buffer);
        username[strlen(nome)-1] = '\0';
        my_send(connected_fd, "nome ok\n");

        my_recv(connected_fd, buffer);
        strcpy(cognome, buffer);
        password[strlen(cognome)-1] = '\0';
        my_send(connected_fd, "cognome ok\n");
		
		my_recv(connected_fd, buffer);
        strcpy(username, buffer);
        username[strlen(username)-1] = '\0';
        my_send(connected_fd, "username ok\n");

        my_recv(connected_fd, buffer);
        strcpy(password, buffer);
        password[strlen(password)-1] = '\0';
        my_send(connected_fd, "password ok\n");

        if ( signup(nome, cognome, username, password) ) {
            my_recv(connected_fd, buffer);
            my_send(connected_fd, "signup ok");
        } else {
            my_recv(connected_fd, buffer);
            my_send(connected_fd, "signup not ok");
        }
    }
    
    //qua dentro metterò gli if delle altre funzioni
    
}

int main(int argc, char *argv[]){
    int listen_fd, connected_fd;
    unsigned int addr_len;
    struct sockaddr_in servaddr;
    struct sockaddr peer;

    printf("Creando le socket...\n"); fflush(stdout);
    if ( (listen_fd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) 
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    printf("Binding the socket...\n"); fflush(stdout);
    // Questa system-call serve ad assegnare un indirizzo locale (name) ad una socket.
    if ( bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) 
        err_sys("bind error");
    
    printf("Sto ascoltando...\n"); fflush(stdout);

    if ( listen(listen_fd, MAX_QUEUE_LEN) < 0 ) 
        err_sys("listen error");

    int current_iteration = 0;
    while (1) {
        // Dopo la chiamata listen(), un server si mette realmente in attesa di connessioni attraverso una chiamata ad accept().
        printf("Accettando la connessione...\n"); fflush(stdout);
        bzero(&peer, sizeof(peer));
        connected_fd = accept(listen_fd, &peer, &addr_len);
        if ( connected_fd < 0 )
            err_sys("accept error"); 
        printf("Connessione accettata\n"); fflush(stdout);
        // Dopo l’accept() il descrittore connected_fd ha la quintupla tutta impostata, ed è pronto ad essere utilizzato.
        // La quintupla è (protocol, local-addr, local-process, foreign-addr, foreign-process)

        if ( fork() == 0 ) {
            close(listen_fd); // child 

            server(connected_fd, peer, addr_len); // adesso si deve processare la richiesta

            close(connected_fd);
            exit(0);
        }

        // Chiude una socket e rilascia le risorse ad essa associate.
        close(connected_fd); // parent 
        current_iteration++;
    }
}
