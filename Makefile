SHELL = /bin/sh

CC = gcc -O

CDEBUG = -g
CFLAGS = $(CDEBUG) -Wall -lfuse 

all:    dlorean travel

dlorean: dlorean.c
	$(CC) -c list.c
	$(CC) -c flux_capacitor.c
	$(CC) -c dlorean.c
	$(CC) $(CFLAGS) -o mount.$@ dlorean.o flux_capacitor.o list.o
 
travel: travel.c
	$(CC) -o $@ travel.c
clean: 
	rm -f *.o mount.dlorean travel core core.*
