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
#define SERVER_ID "-n"
#define SERVER_IP "-j"
#define SERVER_UDP_PORT "-u"
#define SERVER_TCP_PORT "-t"
#define CSERVER_IP "-i"
#define CSERVER_PORT "-p"
#define SERV_OK 1
#define SERV_TROUBLE 0

int main(int argc, char *argv[])
{
  int fd, recv_bytes, sent_bytes, port, arg_count = 0, myid, start_id, id, service, state, myip_arg, myTCPport_arg, myUDPport_arg;
  socklen_t addrlen;
  struct sockaddr_in c_serveraddr, myudpaddr, mytcpaddr, clientaddr;
  struct hostent *hostptr;
  gethostbyname("tejo.tecnico.ulisboa.pt");
  struct in_addr ip;
  char msg[50], buffer[50], req[50], command[20];
  fd = socket(AF_INET,SOCK_DGRAM,0);

  if (argc > 8 && argc % 2)
  {
    memset((void*)&c_serveraddr,(int)'\0', sizeof(c_serveraddr));
    memset((void*)&myudpaddr,(int)'\0', sizeof(myudpaddr));
    memset((void*)&mytcpaddr,(int)'\0', sizeof(mytcpaddr));

    myudpaddr.sin_family = AF_INET;
    mytcpaddr.sin_family = AF_INET;

    c_serveraddr.sin_family = AF_INET;
    hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");
    c_serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
    c_serveraddr.sin_port = htons((u_short)PORT);

    for (int i=1; i<argc; i+=2)
    {
      if(strcmp(argv[i],SERVER_ID)==0)
      {
        myid=atoi(argv[i+1]);
        arg_count++;
      }
      if(strcmp(argv[i],SERVER_IP)==0)
      {
        myip_arg=i;
        inet_aton(argv[i+1],&ip);
        myudpaddr.sin_addr = ip;
        mytcpaddr.sin_addr = ip;
        arg_count++;
      }
      if(strcmp(argv[i],SERVER_UDP_PORT)==0)
      {
        myUDPport_arg=i;
        port=atoi(argv[i+1]);
        myudpaddr.sin_port = htons((u_short)port);
        arg_count++;
      }
      if(strcmp(argv[i],SERVER_TCP_PORT)==0)
      {
        myTCPport_arg=i;
        port=atoi(argv[i+1]);
        mytcpaddr.sin_port = htons((u_short)port);
        arg_count++;
      }
      if(strcmp(argv[i],CSERVER_PORT)==0)
      {
        port=atoi(argv[i+1]);
        c_serveraddr.sin_port = htons((u_short)port);
      }
      if(strcmp(argv[i],CSERVER_IP))
      {
        inet_aton(argv[i+1],&ip);
        c_serveraddr.sin_addr = ip;
      }
    }
    if (arg_count < 3)
    {
      printf("Wrong invocation\n");
      return 0;
    }
  }
  else
  {
    printf("Wrong invocation\n");
    return 0;
  }

  addrlen = sizeof(c_serveraddr);

  bind(fd,(struct sockaddr*)&c_serveraddr, sizeof(c_serveraddr));
  while(1)
  {
    if(scanf("%s\n", req))
    {
      sscanf(req, "%s %d\n", command, &service);
      if (strcmp(command,"join"))
      {
        if (service)
        {
          sprintf(msg,"GET START %d;%d\n",service, myid);
          sent_bytes=sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*)&clientaddr, addrlen);
          if (sent_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          recv_bytes=recvfrom(fd, buffer, sizeof(buffer),0, (struct sockaddr*)&clientaddr, &addrlen);
          if (recv_bytes == -1)
          {
            perror("Error: ");
            state = SERV_TROUBLE;
            break;
          }
          state=sscanf(buffer,"OK %d;%d;%s", &id, &start_id, msg);

          if (state < 3 || id!=myid)
          {
            printf("Trouble contacting the central server\n");
            state = SERV_TROUBLE;
            break;
          }
          if (start_id)
          {
            inet_aton(strtok(msg, ";"),&ip);
            port=atoi(msg);
            //TCP part//
          }
          else
          {
            sprintf(msg, "SET_START %d;%d;%s;%d", service, myid, argv[myip_arg], atoi(argv[myTCPport_arg]));
            sent_bytes=sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*)&clientaddr, addrlen);
            if (sent_bytes == -1)
            {
              perror("Error: ");
              state = SERV_TROUBLE;
              break;
            }
            recv_bytes=recvfrom(fd, buffer, sizeof(buffer),0, (struct sockaddr*)&clientaddr, &addrlen);
            if (recv_bytes == -1)
            {
              perror("Error: ");
              state = SERV_TROUBLE;
              break;
            }
            sscanf(buffer, "OK %d;%*s\n", &id);
            if (id!=myid)
            {
              printf("Trouble contacting the central server\n");
              state = SERV_TROUBLE;
              break;
            }
            sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, argv[myip_arg], atoi(argv[myTCPport_arg]));

          }
        }
        else
          printf("Invalid service id\n");
        if (state == SERV_TROUBLE)
          printf("Couldn't provide the service asked\n");
      else if (strcmp(command,"show_state"))
      {

      }
      else if (strcmp(command,"leave"))
      {

      }
      else if (strcmp(command,"exit"))
      {
        close(fd);
        return 1;
      }
      else
        printf("Unrecognized command\n");
    }
  }
}
//   }
//   recv_bytes=recvfrom(fd, buffer, sizeof(buffer),0, (struct sockaddr*)&clientaddr, &addrlen);
//   if (recv_bytes == -1)
//   {
//     perror("Error: ");
//     return 0;
//   }
//   printf("Received: %s\n", buffer);
//   strncpy(msg,strcat("Echo: ", buffer),strlen(msg));
//   sent_bytes=sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*)&clientaddr, addrlen);
//   while(sent_bytes<strlen(msg)+1)
//   {
//     printf("Sent %d bytes\n", sent_bytes);
//     sent_bytes+=sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*)&clientaddr, addrlen);
//   }
//   close(fd);
// }
