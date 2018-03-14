CC=gcc
CFLAGS=-I.
OBJ = serv_skel.c reqserv.c
EXECUTABLE = serv_skel reqserv

all: serv_skel reqserv

clean:
	rm -f $(EXECUTABLE)
