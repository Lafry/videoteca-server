#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include "rental.h"
#include "json_db.h"
#include "cJSON.h"

extern pthread_mutex_t data_mutex;

void handle_search(int sock, char *params) {
    char *support;
    char *type = strtok_r(params, "|\n", &support);
    char *query = strtok_r(NULL, "|\n", &support);
    if (!type || !query) {
        const char *msg = "Formato comando errato (SEARCH|TIPO|QUERY)\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }

    char reply[2048] = "Risultati ricerca:\n";
    pthread_mutex_lock(&data_mutex);
    int n_films = cJSON_GetArraySize(films_data);

    for (int i = 0; i < n_films; i++) {
        cJSON *film = cJSON_GetArrayItem(films_data, i);
        cJSON *field = NULL;
        if (strcmp(type, "TITLE") == 0)
            field = cJSON_GetObjectItem(film, "titolo");
        else if (strcmp(type, "GENRE") == 0)
            field = cJSON_GetObjectItem(film, "genere");
        
        if (field && strstr(field->valuestring, query) != NULL) {
            char film_info[256];
            int film_id = 0;
            cJSON *id_field = cJSON_GetObjectItem(film, "film_id");
            if (id_field)
                film_id = id_field->valueint;
            cJSON *titolo = cJSON_GetObjectItem(film, "titolo");
            cJSON *genere = cJSON_GetObjectItem(film, "genere");
            cJSON *copies = cJSON_GetObjectItem(film, "copie_disponibili");
            snprintf(film_info, sizeof(film_info),
                     "Film ID: %d, Titolo: %s, Genere: %s, Disponibili: %d\n",
                     film_id,
                     titolo ? titolo->valuestring : "N/A",
                     genere ? genere->valuestring : "N/A",
                     copies ? copies->valueint : 0);
            strcat(reply, film_info);
        }
    }
    pthread_mutex_unlock(&data_mutex);
    strcat(reply, "##END##");
    send(sock, reply, strlen(reply), 0);
}

void handle_rent(int sock, char *params) {
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    char *film_id_str = strtok_r(NULL, "|\n", &support);
    char *return_date = strtok_r(NULL, "|\n", &support);
    if (!username || !film_id_str || !return_date) {
        const char *msg = "Formato comando errato (RENT|username|film_id|return_date)\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }
    int film_id = atoi(film_id_str);
    
    // Ricerca del film e aggiornamento delle copie disponibili
    pthread_mutex_lock(&data_mutex);
    int num_films = cJSON_GetArraySize(films_data);
    int film_found = 0;
    for (int i = 0; i < num_films; i++) {
        cJSON *film = cJSON_GetArrayItem(films_data, i);
        cJSON *id_field = cJSON_GetObjectItem(film, "film_id");
        if (id_field && id_field->valueint == film_id) {
            film_found = 1;
            cJSON *copies_field = cJSON_GetObjectItem(film, "copie_disponibili");
            if (!copies_field || copies_field->valueint <= 0) {
                pthread_mutex_unlock(&data_mutex);
                const char *msg = "Nessuna copia disponibile per questo film!\n##END##";
                send(sock, msg, strlen(msg), 0);
                return;
            }
            int current_copies = copies_field->valueint;
            cJSON_SetIntValue(copies_field, current_copies - 1);
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    if (!film_found) {
        const char *msg = "Film non trovato\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }
    
    // Registrazione del prestito nel record dell'utente – verifica numero massimo di titoli
    pthread_mutex_lock(&data_mutex);
    int user_array_size = cJSON_GetArraySize(users_data);
    for (int i = 0; i < user_array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *user_name = cJSON_GetObjectItem(user, "username");
        if (user_name && strcmp(user_name->valuestring, username) == 0) {
            // Controlla il numero corrente di prestiti dell'utente
            cJSON *prestiti_user = cJSON_GetObjectItem(user, "prestiti");
            int num_loans = prestiti_user ? cJSON_GetArraySize(prestiti_user) : 0;
            if (num_loans >= MAX_LOANS_PER_USER) {
                pthread_mutex_unlock(&data_mutex);
                const char *msg = "Numero massimo di film noleggiati raggiunto\n##END##";
                send(sock, msg, strlen(msg), 0);
                return;
            }
            if (!prestiti_user) {
                prestiti_user = cJSON_CreateArray();
                cJSON_AddItemToObject(user, "prestiti", prestiti_user);
            }
            cJSON *new_prestito = cJSON_CreateObject();
            cJSON_AddNumberToObject(new_prestito, "film_id", film_id);
            cJSON_AddStringToObject(new_prestito, "return_date", return_date);
            cJSON_AddItemToArray(prestiti_user, new_prestito);
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    save_films_data();
    save_users_data();
    
    char reply[100];
    snprintf(reply, sizeof(reply), "Film %d noleggiato con successo\n##END##", film_id);
    send(sock, reply, strlen(reply), 0);
}

void handle_return(int sock, char *params) {
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    char *film_id_str = strtok_r(NULL, "|\n", &support);
    if (!username || !film_id_str) {
        const char *msg = "Formato comando errato (RETURN|username|film_id)\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }
    int film_id = atoi(film_id_str);
    int found = 0;
    
    // Incrementa le copie disponibili per il film restituito
    pthread_mutex_lock(&data_mutex);
    int num_films = cJSON_GetArraySize(films_data);
    for (int i = 0; i < num_films; i++) {
        cJSON *film = cJSON_GetArrayItem(films_data, i);
        cJSON *id_field = cJSON_GetObjectItem(film, "film_id");
        if (id_field && id_field->valueint == film_id) {
            cJSON *copies_field = cJSON_GetObjectItem(film, "copie_disponibili");
            if (copies_field) {
                int current = copies_field->valueint;
                cJSON_SetIntValue(copies_field, current + 1);
            }
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    if (!found) {
        const char *msg = "Film non trovato\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }
    
    // Rimuove il prestito dal record dell'utente
    pthread_mutex_lock(&data_mutex);
    int user_array_size = cJSON_GetArraySize(users_data);
    for (int i = 0; i < user_array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *user_name = cJSON_GetObjectItem(user, "username");
        if (user_name && strcmp(user_name->valuestring, username) == 0) {
            cJSON *prestiti_user = cJSON_GetObjectItem(user, "prestiti");
            if (prestiti_user) {
                int num_prestiti = cJSON_GetArraySize(prestiti_user);
                int index_to_delete = -1;
                for (int j = 0; j < num_prestiti; j++) {
                    cJSON *prestito = cJSON_GetArrayItem(prestiti_user, j);
                    cJSON *film_field = cJSON_GetObjectItem(prestito, "film_id");
                    if (film_field && film_field->valueint == film_id) {
                        index_to_delete = j;
                        break;
                    }
                }
                if (index_to_delete != -1) {
                    cJSON_DeleteItemFromArray(prestiti_user, index_to_delete);
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    save_films_data();
    save_users_data();
    const char *msg = "Film restituito con successo\n##END##";
    send(sock, msg, strlen(msg), 0);

}

void handle_my_rentals(int sock, char *params) {
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    if (!username) {
        const char *msg = "Formato comando errato (MYRENTALS|username)\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }
    char reply[2048] = "Film noleggiati:\n";
    pthread_mutex_lock(&data_mutex);
    int user_array_size = cJSON_GetArraySize(users_data);
    for (int i = 0; i < user_array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *user_name = cJSON_GetObjectItem(user, "username");
        if (user_name && strcmp(user_name->valuestring, username) == 0) {
            cJSON *prestiti_user = cJSON_GetObjectItem(user, "prestiti");
            if (prestiti_user) {
                int num_prestiti = cJSON_GetArraySize(prestiti_user);
                for (int j = 0; j < num_prestiti; j++) {
                    cJSON *prestito = cJSON_GetArrayItem(prestiti_user, j);
                    cJSON *film_id_item = cJSON_GetObjectItem(prestito, "film_id");
                    cJSON *return_date_item = cJSON_GetObjectItem(prestito, "return_date");
                    if (film_id_item && return_date_item) {
                        int film_id = film_id_item->valueint;
                        char film_title[100] = "N/D";
                        // Cerca nella lista dei film per ottenere il titolo corrispondente all'ID.
                        int num_films = cJSON_GetArraySize(films_data);
                        for (int k = 0; k < num_films; k++) {
                            cJSON *film = cJSON_GetArrayItem(films_data, k);
                            cJSON *id_field = cJSON_GetObjectItem(film, "film_id");
                            if (id_field && id_field->valueint == film_id) {
                                cJSON *titolo_item = cJSON_GetObjectItem(film, "titolo");
                                if (titolo_item)
                                    strncpy(film_title, titolo_item->valuestring, sizeof(film_title) - 1);
                                break;
                            }
                        }
                        char film_info[256];
                        snprintf(film_info, sizeof(film_info),
                                 "Film ID: %d, Titolo: %s, da restituire entro: %s\n",
                                 film_id, film_title, return_date_item->valuestring);
                        strcat(reply, film_info);
                    }
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    strcat(reply, "##END##");
    send(sock, reply, strlen(reply), 0);
}
