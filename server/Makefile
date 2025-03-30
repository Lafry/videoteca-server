# Makefile per il server

CC = gcc
CFLAGS = -Wall -pthread -D_XOPEN_SOURCE
LIBS = -lm

SRC = main.c json_db.c auth.c cJSON.c
TARGET = server

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

