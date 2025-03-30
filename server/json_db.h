#ifndef JSON_DB_H
#define JSON_DB_H

#include <pthread.h>
#include "cJSON.h"

// Percorsi dei file JSON (i file verranno montati tramite bind mount)
extern const char* users_filename;
extern const char* films_filename;

// Variabili globali per i dati (oggetti JSON)
extern cJSON *users_data;
extern cJSON *films_data;

// Mutex per sincronizzare gli accessi a questi dati
extern pthread_mutex_t data_mutex;

// Funzioni per caricare e salvare i dati JSON
void load_users_data();
void load_films_data();
void save_users_data();
void save_films_data();

#endif
