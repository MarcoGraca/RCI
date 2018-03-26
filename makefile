CC=gcc
CFLAGS=-I.
OBJ = service.c reqserv.c
EXECUTABLE = service reqserv

all: service reqserv

clean:
	rm -f $(EXECUTABLE)
