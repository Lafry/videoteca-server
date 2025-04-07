#ifndef AUTH_H
#define AUTH_H

void handle_register(int sock, char *params);
void handle_login(int sock, char *params);

#endif
