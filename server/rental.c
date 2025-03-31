#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include "rental.h"
#include "json_db.h"
#include "cJSON.h"

#define MAX_FILM_PER_USER 3

void handle_search(int sock, char *params) {
    char *saveptr;
    char *type = strtok_r(params, "|\n", &saveptr);
    char *query = strtok_r(NULL, "|\n", &saveptr);
    if (!type || !query) {
        const char *msg = "Formato comando errato (SEARCH|TIPO|QUERY)\n##END##";
        send(sock, msg, strlen(msg), 0);
        return;
    }
    printf("Sto procedendo alla ricerca");
    fflush(stdout);
    char reply[2048] = "Risultati ricerca:\n";
    pthread_mutex_lock(&data_mutex);
    int n_films = cJSON_GetArraySize(films_data);
    printf("%d film trovati", &n_films);
    fflush(stdout);
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
