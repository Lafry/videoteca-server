#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "json_db.h"
#include "cJSON.h"

cJSON *users_data = NULL;
cJSON *films_data = NULL;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

void load_films_data() {
    FILE *fp = fopen("films.json", "r");
    if (!fp) {
        perror("Errore apertura films.json");
        // Inizializza un array vuoto
        films_data = cJSON_CreateArray();
        return;
    }
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);
    char *data = malloc(length + 1);
    if (data) {
        fread(data, 1, length, fp);
        data[length] = '\0';
        films_data = cJSON_Parse(data);
        free(data);
    }
    fclose(fp);
    if (!films_data) {
        films_data = cJSON_CreateArray();
    }
}

void load_users_data() {
    FILE *fp = fopen("users.json", "r");
    if (!fp) {
        perror("Errore apertura users.json");
        // Inizializza un array vuoto
        users_data = cJSON_CreateArray();
        return;
    }
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);
    char *data = malloc(length + 1);
    if (data) {
        fread(data, 1, length, fp);
        data[length] = '\0';
        users_data = cJSON_Parse(data);
        free(data);
    }
    fclose(fp);
    if (!users_data) {
        users_data = cJSON_CreateArray();
    }
}

void save_films_data() {
    FILE *fp = fopen("films.json", "w");
    if (!fp) {
        perror("Errore apertura films.json per scrittura");
        return;
    }
    char *printed = cJSON_Print(films_data);
    if (printed) {
        fprintf(fp, "%s", printed);
        free(printed);
    }
    fclose(fp);
}

void save_users_data() {
    FILE *fp = fopen("users.json", "w");
    if (!fp) {
        perror("Errore apertura users.json per scrittura");
        return;
    }
    char *printed = cJSON_Print(users_data);
    if (printed) {
        fprintf(fp, "%s", printed);
        free(printed);
    }
    fclose(fp);
}
