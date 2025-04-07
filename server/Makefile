# Makefile per il server

CC = gcc
CFLAGS = -Wall -pthread -D_XOPEN_SOURCE
LIBS = -lm

SRC = main.c json_db/json_db.c auth/auth.c rental/rental.c cJSON/cJSON.c
TARGET = server

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

