#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>
#include "auth.h"
#include "json_db.h"
#include "cJSON.h"
#include "rental.h"

extern pthread_mutex_t data_mutex;

void handle_register(int sock, char *params) {
    // Parametri attesi: "username|password"
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    char *password = strtok_r(NULL, "|\n", &support);
    if (!username || !password) {
        const char *error = "Formato comando errato (REGISTER|username|password_hash)\n##END##";
        send(sock, error, strlen(error), 0);
        return;
    }
    
    pthread_mutex_lock(&data_mutex);
    // Controlla se l'utente esiste gia'
    int user_array_size = cJSON_GetArraySize(users_data);
    for (int i = 0; i < user_array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *name = cJSON_GetObjectItem(user, "username");
        if (name && strcmp(name->valuestring, username) == 0) {
            pthread_mutex_unlock(&data_mutex);
            const char *msg = "Username gia' registrato\n##END##";
            send(sock, msg, strlen(msg), 0);
            return;
        }
    }
    
    //Crea il nuovo utente
    cJSON *new_user = cJSON_CreateObject();
    cJSON_AddStringToObject(new_user, "username", username);
    cJSON_AddStringToObject(new_user, "password", password);
    cJSON_AddItemToObject(new_user, "prestiti", cJSON_CreateArray());
    cJSON_AddItemToArray(users_data, new_user);
    pthread_mutex_unlock(&data_mutex);
    
    save_users_data();
    const char *successMessage = "Registrazione avvenuta con successo\n##END##";
    send(sock, successMessage, strlen(successMessage), 0);
}

void handle_login(int sock, char *params) {
    //Parametri attesi: "username|password"
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    char *password = strtok_r(NULL, "|\n", &support);
    if (!username || !password) {
        const char *message = "Formato comando errato (LOGIN|username|password_hash)\n##END##";
        send(sock, message, strlen(message), 0);
        return;
    }
    
    int authenticated = 0;
    pthread_mutex_lock(&data_mutex);
    int array_size = cJSON_GetArraySize(users_data);
    cJSON *logged_user = NULL;
    for (int i = 0; i < array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *name = cJSON_GetObjectItem(user, "username");
        cJSON *pass = cJSON_GetObjectItem(user, "password");
        if (name && pass && strcmp(name->valuestring, username) == 0 &&
            strcmp(pass->valuestring, password) == 0) {
            authenticated = 1;
            // Salva il riferimento all'utente
            logged_user = user;
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    if (authenticated) {
        char message[1024];
        snprintf(message, sizeof(message), "Login effettuato\n");
        
        // Controlla se l'utente ha prestiti con data di restituzione in scadenza
        if (logged_user) {
            cJSON *prestiti = cJSON_GetObjectItem(logged_user, "prestiti");
            if (prestiti && cJSON_GetArraySize(prestiti) > 0) {
                time_t now = time(NULL);
                for (int i = 0; i < cJSON_GetArraySize(prestiti); i++) {
                    cJSON *prestito = cJSON_GetArrayItem(prestiti, i);
                    cJSON *return_date_item = cJSON_GetObjectItem(prestito, "return_date");
                    cJSON *film_id_item = cJSON_GetObjectItem(prestito, "film_id");
                    if (return_date_item && film_id_item) {
                        struct tm return_tm;
                        memset(&return_tm, 0, sizeof(struct tm));
                        // Prova a interpretare la data in formato "YYYY-MM-DD"
                        if (strptime(return_date_item->valuestring, "%Y-%m-%d", &return_tm) != NULL) {
                            time_t return_time = mktime(&return_tm);
                            double diff = difftime(return_time, now);
                            if (diff <= (WARNING_DAYS * 24 * 3600)) {
                                // Recupera il titolo del film cercando in films_data
                                int film_id = film_id_item->valueint;
                                char film_title[100] = "Sconosciuto";
                                pthread_mutex_lock(&data_mutex);
                                int num_films = cJSON_GetArraySize(films_data);
                                for (int j = 0; j < num_films; j++) {
                                    cJSON *film = cJSON_GetArrayItem(films_data, j);
                                    cJSON *id_field = cJSON_GetObjectItem(film, "film_id");
                                    if (id_field && id_field->valueint == film_id) {
                                        cJSON *titolo = cJSON_GetObjectItem(film, "titolo");
                                        if (titolo)
                                            strncpy(film_title, titolo->valuestring, sizeof(film_title)-1);
                                        break;
                                    }
                                }
                                pthread_mutex_unlock(&data_mutex);
                                
                                char warning[256];
                                snprintf(warning, sizeof(warning),
                                         "Attenzione: il film '%s' deve essere restituito entro %s\n",
                                         film_title, return_date_item->valuestring);
                                strncat(message, warning, sizeof(message) - strlen(message) - 1);
                            }
                        }
                    }
                }
            }
        }
        strncat(message, "##END##", sizeof(message) - strlen(message) - 1);
        send(sock, message, strlen(message), 0);
    } else {
        const char *msg = "Username o password errati\n##END##";
        send(sock, msg, strlen(msg), 0);
    }
}
