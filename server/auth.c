#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>
#include "auth.h"
#include "json_db.h"
#include "cJSON.h"

#define _XOPEN_SOURCE
#define NOTIFICATION_EXPIRATION_DAYS 7

void handle_register(int sock, char *params) {
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    char *password = strtok_r(NULL, "|\n", &support);
    if (!username || !password) {
        const char *error = "Formato comando errato (REGISTER|username|password_hash)\n##END##";
        send(sock, error, strlen(error), 0);
        return;
    }
    
    int exists = 0;
    pthread_mutex_lock(&data_mutex);
    int array_size = cJSON_GetArraySize(users_data);
    for (int i = 0; i < array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *u = cJSON_GetObjectItem(user, "username");
        if (u && strcmp(u->valuestring, username) == 0) {
            exists = 1;
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    if (exists) {
        const char *message = "Username gia' esistente!\n##END##";
        send(sock, message, strlen(message), 0);
        return;
    }
    
    pthread_mutex_lock(&data_mutex);
    cJSON *new_user = cJSON_CreateObject();
    cJSON_AddStringToObject(new_user, "username", username);
    cJSON_AddStringToObject(new_user, "password", password);
    cJSON_AddItemToObject(new_user, "prestiti", cJSON_CreateArray());
    cJSON_AddItemToArray(users_data, new_user);
    pthread_mutex_unlock(&data_mutex);
    
    save_users_data();
    const char *successMessage = "Registrazione avvenuta con successo\n##END##";
    send(sock, successMessage, strlen(successMsg), 0);
}

void handle_login(int sock, char *params) {
    char *support;
    char *username = strtok_r(params, "|\n", &support);
    char *password_hash = strtok_r(NULL, "|\n", &support);
    if (!username || !password) {
        const char *message = "Formato comando errato (LOGIN|username|password_hash)\n##END##";
        send(sock, message, strlen(message), 0);
        return;
    }
    
    int authenticated = 0;
    pthread_mutex_lock(&data_mutex);
    int array_size = cJSON_GetArraySize(users_data);
    cJSON *user_obj = NULL;
    for (int i = 0; i < array_size; i++) {
        cJSON *user = cJSON_GetArrayItem(users_data, i);
        cJSON *u = cJSON_GetObjectItem(user, "username");
        cJSON *p = cJSON_GetObjectItem(user, "password");
        if (u && p && strcmp(u->valuestring, username) == 0 &&
            strcmp(p->valuestring, password) == 0) {
            authenticated = 1;
            user_obj = user;
            break;
        }
    }
    pthread_mutex_unlock(&data_mutex);
    
    if (!authenticated) {
        const char *error = "Credenziali errate\n##END##";
        send(sock, error, strlen(error), 0);
        return;
    }
    
    char final_response[1024];
    snprintf(final_response, sizeof(final_response),
             "Login effettuato\n%s##END##", notification);
    send(sock, final_response, strlen(final_response), 0);
}
