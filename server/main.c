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

void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);
    char buffer[1024];
    int read_size;
    
    while ((read_size = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("Ricevuto: %s\n", buffer);
        
        char *suppport;
        char *command = strtok_r(buffer, "|\n", &support);
        if (!command) {
            send(sock, "Comando non riconosciuto\n", 26, 0);
            continue;
        }
        if (strcmp(command, "REGISTER") == 0) {
            handle_register(sock, support);
        } else if (strcmp(command, "LOGIN") == 0) {
            handle_login(sock, suppor);
        } else if (strcmp(command, "SEARCH") == 0) {
            handle_search(sock, saveptr);
        } else {
            send(sock, "Comando non riconosciuto\n", 26, 0);
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

    setbuf(stdout, NULL);
    load_users_data();
    load_films_data();
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fallito");
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
    
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen))) {
        pthread_t thread;
        new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        if (pthread_create(&thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Impossibile creare thread");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread);
    }
    
    if (new_socket < 0) {
        perror("Accept fallita");
        exit(EXIT_FAILURE);
    }
    
    cJSON_Delete(users_data);
    cJSON_Delete(films_data);
    pthread_mutex_destroy(&data_mutex);
    return 0;
}
