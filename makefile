OBJ = contacts.o
EXECUTABLE = service reqserv

all: service reqserv

service: service.c contacts.o contacts.h
	gcc -g -o service contacts.o service.c

reqserv: reqserv.c contacts.o
	gcc -g -o reqserv contacts.o reqserv.c

clean:
	rm -f $(EXECUTABLE) $(OBJ)
