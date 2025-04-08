#ifndef RENTAL_H
#define RENTAL_H

// Numero massimo di titoli che un utente può avere noleggiati contemporaneamente
#define MAX_LOANS_PER_USER 10
#define DEFAULT_MAX_POPULAR_SIZE_LIST 5
// Se la data di restituzione è entro WARNING_DAYS giorni dalla data odierna, invia un avviso
#define WARNING_DAYS 7

// Gestisce la ricerca di film (in base al titolo o genere)
void handle_search(int sock, char *params);

// Gestisce il noleggio di un film; registra il prestito nell'utente (users.json)
// e decrementa le copie disponibili in films_data.
void handle_rent(int sock, char *params);

// Gestisce la restituzione di un film; rimuove il prestito dall'utente
// e incrementa le copie disponibili nel record del film.
void handle_return(int sock, char *params);

// Restituisce l'elenco dei film noleggiati per un utente.
void handle_my_rentals(int sock, char *params);

#endif
