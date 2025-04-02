#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "json_db.h"
#include "auth.h"
#include "rental.h"

#define PORT 8080
#define MAX_CONN 10

// Funzione che viene eseguita da ogni thread per gestire le richieste client
void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);
    char buffer[1024];
    int read_size;
    
    while ((read_size = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("Ricevuto: %s\n", buffer);
        
        char *support;
        char *command = strtok_r(buffer, "|\n", &support);
        if (!command) {
            const char *msg = "Comando non riconosciuto\n##END##";
            send(sock, msg, strlen(msg), 0);
            continue;
        }
        if (strcmp(command, "REGISTER") == 0) {
            handle_register(sock, support);
        } else if (strcmp(command, "LOGIN") == 0) {
            handle_login(sock, support);
        } else if (strcmp(command, "SEARCH") == 0) {
            handle_search(sock, support);
        }  else if (strcmp(command, "RENT") == 0) {
            handle_rent(sock, support);
        } else if (strcmp(command, "RETURN") == 0) {
            handle_return(sock, support);
        } else if (strcmp(command, "MYRENTALS") == 0) {
            handle_my_rentals(sock, support);
        } else {
            const char *msg = "Comando non riconosciuto\n##END##";
            send(sock, msg, strlen(msg), 0);
        }
    }
    
    if (read_size == 0)
        puts("Client disconnesso");
    else if (read_size < 0)
        perror("recv fallita");
    
    close(sock);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Imposta il buffer per i log del server sul container a null, in modo da avere log stampati appena vengono scritti
    setbuf(stdout, NULL);
    
    //Carica i dati JSON all'avvio
    load_users_data();
    load_films_data();
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Errore nell'apertura del socket");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fallito");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, MAX_CONN) < 0) {
        perror("Listen fallita");
        exit(EXIT_FAILURE);
    }
    
    printf("Server in ascolto sulla porta %d...\n", PORT);
    
    //per ogni nuova connessione accettata, crea un nuovo thread
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen))) {
        pthread_t thread;
        new_sock = malloc(sizeof(int));
        if (!new_sock) {
            perror("Errore nell'allocazione memoria");
            continue;
        }
        *new_sock = new_socket;
        if (pthread_create(&thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Impossibile creare thread");
            exit(EXIT_FAILURE);
        }
        
        //stacco il thread, in modo da non doverlo gestire successivamente per la chiusura
        pthread_detach(thread);
    }
    
    if (new_socket < 0) {
        perror("Accept fallita");
        exit(EXIT_FAILURE);
    }
    
    // Pulizia finale: libera i dati JSON e distrugge il mutex
    cJSON_Delete(users_data);
    cJSON_Delete(films_data);
    pthread_mutex_destroy(&data_mutex);
    
    return 0;
}
