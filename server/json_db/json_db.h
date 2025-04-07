#ifndef JSON_DB_H
#define JSON_DB_H

#include <pthread.h>
#include "../cJSON/cJSON.h"

// Variabile contenente i dati del json users.json
extern cJSON *users_data;

// Variabile contenente i dati del json films.json
extern cJSON *films_data;

// Mutex per sincronizzare gli accessi a questi dati
extern pthread_mutex_t data_mutex;

// Funzioni per caricare e salvare i dati JSON
void load_users_data();
void load_films_data();
void save_users_data();
void save_films_data();

#endif
