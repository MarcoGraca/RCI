#ifndef CONTACTS_H
#define CONTACTS_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define PORT 59000
#define SERVER_ID "-n"
#define SERVER_IP "-j"
#define SERVER_UDP_PORT "-u"
#define SERVER_TCP_PORT "-t"
#define CSERVER_IP "-i"
#define CSERVER_PORT "-p"
#define SERV_OK 1
#define SERV_TROUBLE 0
#define JOINING_DISPATCH "MY SERVICE ON"
#define JOINED_DISPATCH "YOUR SERVICE ON"
#define LEAVING_DISPATCH "MY SERVICE OFF"
#define LEFT_DISPATCH "YOUR SERVICE OFF"
#define BUFFERSIZE 50
#define max(A,B) ((A)>=(B)?(A):(B))

int UDP_contact(char *msg, struct sockaddr_in serveraddr, int afd, char *buffer);
int TCP_contact(char *msg, int afd, char *buffer);
#endif
