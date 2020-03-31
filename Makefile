CC = gcc
CFLAGS = -g -O2 -Wall -Wundef
OBJECTS =

all: client server test

client: client.c utils.o
	$(CC) $(CFLAGS) -o client client.c utils.o -lpthread
	$(CC) -o test test.c

server: server.c utils.o kissdb.o queue.h
	$(CC) $(CFLAGS) -o server server.c utils.o kissdb.o -lpthread

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o test client server *.db 
