#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_db.h"
#include "cJSON.h"

const char* users_filename = "users.json";

cJSON *users_data = NULL;
cJSON *films_data = NULL;

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* load_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Errore apertura file");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char *data = malloc(length + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }
    fread(data, 1, length, f);
    data[length] = '\0';
    fclose(f);
    return data;
}

static int save_file(const char* filename, const char* data) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Errore apertura file per scrittura");
        return -1;
    }
    fwrite(data, sizeof(char), strlen(data), f);
    fclose(f);
    return 0;
}

void load_users_data() {
    char *data = load_file(users_filename);
    if (data == NULL) {
        users_data = cJSON_CreateArray();
        char *printed = cJSON_Print(users_data);
        save_file(users_filename, printed);
        free(printed);
        printf("Creato nuovo file JSON per gli utenti.\n");
	fflus(stdout);
    } else {
        users_data = cJSON_Parse(data);
        if (!users_data) {
            printf("Errore nel parsing di users.json. Creo nuova struttura base.\n");
            fflush(stdout);
	    users_data = cJSON_CreateArray();
        }
        free(data);
    }
}

void save_users_data() {
    pthread_mutex_lock(&data_mutex);
    char *printed = cJSON_Print(users_data);
    save_file(users_filename, printed);
    free(printed);
    pthread_mutex_unlock(&data_mutex);
}
