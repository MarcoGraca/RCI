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

int central_contact(char *msg, struct sockaddr_in c_serveraddr, int fd, char *buffer)
{
  socklen_t addrlen;
  int sent_bytes, recv_bytes, count;
  struct timeval countdwn;
  countdwn.tv_sec=5;
  countdwn.tv_usec=0;
  fd_set rfds;
  printf("Sending: %s (%lu bytes) to port %d of %s\n",msg, strlen(msg),c_serveraddr.sin_port, inet_ntoa(c_serveraddr.sin_addr));
  sent_bytes=sendto(fd, msg, strlen(msg), 0, (struct sockaddr*)&c_serveraddr, sizeof(c_serveraddr));
  if (sent_bytes == -1)
  {
    perror("Error: ");
    return(SERV_TROUBLE);
  }
  printf("SENT %d BYTES\n",sent_bytes);
  FD_ZERO(&rfds);
  FD_SET(fd,&rfds);
  count=select(fd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,&countdwn);
  if (count < 1 || countdwn.tv_sec==0)
  {
    printf("SELECT TROUBLE\n");
    return (SERV_TROUBLE);
  }
  addrlen = sizeof(c_serveraddr);
  recv_bytes=recvfrom(fd, buffer, sizeof(buffer),0, (struct sockaddr*)&c_serveraddr, &addrlen);
  if (recv_bytes == -1)
  {
    perror("Error: ");
    return(SERV_TROUBLE);
  }
  printf("RECEIVED %d BYTES\n",recv_bytes);
  return(SERV_OK);
}

int main(int argc, char *argv[])
{
  int i,fd, clfd, recv_bytes, sent_bytes, port, arg_count = 0, myid, start_id, id, service, state, myip_arg, myTCPport_arg, myUDPport_arg, counter, maxfd;
  fd_set rfds, wfds;
  socklen_t addrlen;
  struct sockaddr_in c_serveraddr, myudpaddr, mytcpaddr, clientaddr;
  struct in_addr ip;
  char msg[BUFFERSIZE], buffer[BUFFERSIZE], req[BUFFERSIZE], command[BUFFERSIZE];
  enum {idle, contact, on_ring, busy} status;
  enum {ring_join, ring_wd, client_join, client_wd} cont;
  fd = socket(AF_INET,SOCK_DGRAM,0);
  clfd = socket(AF_INET,SOCK_DGRAM,0);
  if (argc > 8 && argc % 2)
  {
    memset((void*)&c_serveraddr,(int)'\0', sizeof(c_serveraddr));
    memset((void*)&myudpaddr,(int)'\0', sizeof(myudpaddr));
    memset((void*)&mytcpaddr,(int)'\0', sizeof(mytcpaddr));

    myudpaddr.sin_family = AF_INET;
    mytcpaddr.sin_family = AF_INET;

    c_serveraddr.sin_family = AF_INET;
    inet_aton("193.136.138.142",&ip);
    c_serveraddr.sin_addr = ip;
    c_serveraddr.sin_port = htons((u_short)PORT);

    for (i=1; i<argc; i+=2)
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
      if(strcmp(argv[i],CSERVER_IP)==0)
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
  status=idle;
  printf("Sending to port %d of %s\n",c_serveraddr.sin_port,inet_ntoa(c_serveraddr.sin_addr));
  while(1)
  {
    FD_ZERO(&rfds);
    FD_SET(0,&rfds);
    maxfd=0;
    if (status == on_ring || status == busy)
    {
      FD_SET(clfd,&rfds);
      maxfd=clfd;
    }
    counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
    if (counter < 1)
      return 0;
    if (FD_ISSET(0,&rfds))
    {
      switch (status)
      {
        case idle:
        {
          if(fgets(req,BUFFERSIZE,stdin)==NULL)
            printf("Couldn't read from terminal\n");
          else
          {
            sscanf(req, "%s %d\n", command, &service);
            printf("Service: %d\n", service);
            if (strcmp(command,"join")==0)
            {
              if(service)
              {
                status=contact;
                cont=ring_join;
              }
              else
                printf("Invalid service id\n");
            }
            else if (strcmp(command,"show_state")==0)
              printf("TESTE\n");
            else if (strcmp(command,"leave")==0)
              printf("Not in a ring\n");
            else if (strcmp(command,"exit")==0)
            {
              close(fd);
              return 1;
            }
            else
              printf("Unrecognized command\n");
          }
        }
        case contact:
        {
          switch (cont)
          {
            case ring_join:
            {
              sprintf(msg,"GET START %d;%d\n",service, myid);
              state=central_contact(msg, c_serveraddr,fd,buffer);
              if (state==SERV_TROUBLE)
              {
                printf("Trouble contacting the central server\n");
                break;
              }
              i=sscanf(buffer,"OK %d;%d;%s", &id, &start_id, msg);
              if (i < 3 || id!=myid)
              {
                printf("Trouble querying the central server\n");
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
                //TCP stuff//
                sprintf(msg, "SET_START %d;%d;%s;%d", service, myid, argv[myip_arg], atoi(argv[myTCPport_arg]));
                state=central_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  break;
                }
                i=sscanf(buffer, "OK %d;%*s\n", &id);
                if (i < 1 || id!=myid)
                {
                  printf("Trouble querying the central server\n");
                  break;
                }
                sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, argv[myip_arg], atoi(argv[myUDPport_arg]));
                state=central_contact(msg, c_serveraddr,fd,buffer);
                i=sscanf(buffer, "OK %d;%*s\n", &id);
                if (i < 1 || id!=myid)
                {
                  printf("Trouble querying the central server\n");
                  break;
                }
                status=on_ring;
              }
            }
            case ring_wd:
            {
              break;
            }
            case client_join:
            {
              break;
            }
            case client_wd:
            {
              break;
            }
          }
        }
        case on_ring:
        {
          if(fgets(req,BUFFERSIZE,stdin)==NULL)
          {
            //TCP stuff
            break;
          }
          else
          {
            sscanf(req, "%s\n", command);
            if (strcmp(req,JOINING_DISPATCH)==0)
            {
              //TCP stuff here too probably
              status=contact;
              cont=client_join;
            }
          }
        }
        case busy:
        {
          if(FD_ISSET(clfd,&rfds)&&FD_ISSET(clfd+1,&wfds))
          {
            recv_bytes=recvfrom(clfd, buffer, sizeof(buffer),0, (struct sockaddr*)&clientaddr, &addrlen);
            if (recv_bytes == -1)
            {
              perror("Error: ");
              state = SERV_TROUBLE;
              break;
            }
            if (strcmp(req,LEAVING_DISPATCH)==0)
            {
              sprintf(msg, LEFT_DISPATCH);
              sent_bytes=sendto(clfd, msg, strlen(msg)+1, 0, (struct sockaddr*)&clientaddr, addrlen);
              if (sent_bytes == -1)
              {
                perror("Error: ");
                state = SERV_TROUBLE;
                break;
              }
            }
          }
        }
      }
    }
    if (FD_ISSET(clfd,&rfds))
    {

    }
  }
}
