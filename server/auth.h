#ifndef AUTH_H
#define AUTH_H

// Adesso il client invia la password già hashata
// La sintassi prevista è:
//   REGISTER|username|password_hash\n
//   LOGIN|username|password_hash\n

void handle_register(int sock, char *params);
void handle_login(int sock, char *params);

#endif
