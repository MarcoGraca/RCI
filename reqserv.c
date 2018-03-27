#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#define PORT 59000
#define CSIP "-i"
#define CSPT "-p"
#define JOINING_DISPATCH "MY SERVICE ON"
#define JOINED_DISPATCH "YOUR SERVICE ON"
#define LEAVING_DISPATCH "MY SERVICE OFF"
#define LEFT_DISPATCH "YOUR SERVICE OFF"
#define SERV_OK 1
#define SERV_TROUBLE 0

int main(int argc, char* argv[])
{
  char req[50], command[25], msg[50], buffer[50];
  struct hostent *hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");
  struct in_addr dsip;
  fd_set rfds;
  FD_ZERO(&rfds);
  int fd = socket(AF_INET,SOCK_DGRAM,0), dsfd = socket(AF_INET,SOCK_DGRAM,0), recv_bytes, sent_bytes, service=0, port=PORT, dsid, dspt, state;
  socklen_t addrlen;
  struct sockaddr_in c_serveraddr, d_serveraddr;
  if (argc%2 && argc < 6)
  {
    if(strcmp(argv[1],CSIP)==0)
      hostptr = gethostbyname(argv[2]);
    if(strcmp(argv[1],CSPT)==0)
      port = atoi(argv[2]);
    if (argc == 5)
    {
      if(strcmp(argv[3],CSIP)==0)
        hostptr = gethostbyname(argv[4]);
      if(strcmp(argv[3],CSPT)==0)
        port = atoi(argv[4]);
    }
  }
  else
  {
    printf("Wrong invocation\n");
    return 0;
  }
  memset((void*)&c_serveraddr,(int)'\0', sizeof(c_serveraddr));
  c_serveraddr.sin_family = AF_INET;
  c_serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
  c_serveraddr.sin_port = htons((u_short)port);
  addrlen = sizeof(c_serveraddr);
  while(1)
  {
    if (fgets(req, strlen(req), stdin)!= NULL)
    {
      sscanf(req, "%s %d\n", command, &service);
      if (strcmp(command,"request_service")==0||strcmp(command,"rs")==0)
      {
        if (service)
        {
          sprintf(msg, "GET_DS_SERVER %d",service);
          sent_bytes=sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*)&c_serveraddr, addrlen);
          if (sent_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          addrlen = sizeof(c_serveraddr);
          recv_bytes=recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&c_serveraddr, &addrlen);
          if (recv_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          sscanf(buffer, "OK %d;%s", &dsid, msg);
          printf("Dispatch server id: %d\n", dsid);
          inet_aton(strtok(msg, ";"),&dsip);
          dspt=atoi(msg);
          memset((void*)&d_serveraddr,(int)'\0', sizeof(d_serveraddr));
          d_serveraddr.sin_family = AF_INET;
          d_serveraddr.sin_addr = dsip;
          d_serveraddr.sin_port = htons((u_short)dspt);
          addrlen = sizeof(d_serveraddr);
          sent_bytes=sendto(dsfd, JOINING_DISPATCH, strlen(JOINING_DISPATCH)+1, 0, (struct sockaddr*)&d_serveraddr, addrlen);
          if (sent_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          recv_bytes=recvfrom(dsfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&d_serveraddr, &addrlen);
          if (sent_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          if (strcmp(buffer,JOINED_DISPATCH))
          {
            state = SERV_OK;
            printf("Service successfully initialized\n");
          }
          else
            state = SERV_TROUBLE;
        }
        else
          printf("Invalid service id\n");
        if (state == SERV_TROUBLE)
          printf("Couldn't initialize the service\n");
      }
      else if(strcmp(command,"terminate_service")==0||strcmp(command,"ts")==0)
      {
        if (service)
        {
          sent_bytes=sendto(dsfd, LEAVING_DISPATCH, strlen(LEAVING_DISPATCH)+1, 0, (struct sockaddr*)&d_serveraddr, addrlen);
          if (sent_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          recv_bytes=recvfrom(dsfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&d_serveraddr, &addrlen);
          if (sent_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          if (strcmp(buffer,LEFT_DISPATCH))
          {
            state = SERV_OK;
            close(dsfd);
            printf("Service successfully terminated\n");
          }
          else
            state = SERV_TROUBLE;
        }
        else
          printf("No service running to terminate\n");
        if (state == SERV_TROUBLE)
          printf("Couldn't terminate the service\n");
      }
      else if(strcmp(command,"exit")==0)
      {
        close(fd);
        return 1;
      }
      else
        printf("Unrecognized command\n");
    }
  }
}
