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
  int sent_bytes, recv_bytes;
  struct timeval cntdwn;
  cntdwn.tv_sec=3;
  cntdwn.tv_usec=0;
  fd_set irfds;
  sent_bytes=sendto(fd, msg, strlen(msg), 0, (struct sockaddr*)&c_serveraddr, sizeof(c_serveraddr));
  if (sent_bytes == -1)
  {
    perror("Error: ");
    return SERV_TROUBLE;
  }
  printf("SENT: %s (%d BYTES)\n",msg,sent_bytes);
  FD_ZERO(&irfds);
  FD_SET(fd,&irfds);
  select(fd+1,&irfds,(fd_set*)NULL,(fd_set*)NULL,&cntdwn);
  if (FD_ISSET(fd,&irfds))
  {
    addrlen = sizeof(c_serveraddr);
    recv_bytes=recvfrom(fd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&c_serveraddr, &addrlen);
    if (recv_bytes == -1)
    {
      perror("Error: ");
      return SERV_TROUBLE;
    }
    buffer[recv_bytes]='\0';
    printf("RECEIVED: %s (%d BYTES)\n",buffer,recv_bytes);
    return SERV_OK;
  }
  else
  {
    printf("SELECT TROUBLE\n");
    return SERV_TROUBLE;
  }
}

int main(int argc, char *argv[])
{
  int i, fd, clfd, recv_bytes, sent_bytes, port, arg_count = 0, myid, start_id, id, service, state, myip_arg, myTCPport_arg, myUDPport_arg, counter, maxfd;
  fd_set rfds;
  socklen_t addrlen;
  struct sockaddr_in c_serveraddr, myudpaddr, mytcpaddr, clientaddr;
  struct in_addr ip;
  struct hostent *hostptr;
  char msg[BUFFERSIZE], buffer[BUFFERSIZE], req[BUFFERSIZE], command[BUFFERSIZE];
  enum {idle, on_ring, busy} status;
  fd = socket(AF_INET,SOCK_DGRAM,0);
  clfd = socket(AF_INET,SOCK_DGRAM,0);
  if (argc > 8 && argc % 2)
  {
    memset((void*)&c_serveraddr,(int)'\0', sizeof(c_serveraddr));
    memset((void*)&myudpaddr,(int)'\0', sizeof(myudpaddr));
    memset((void*)&mytcpaddr,(int)'\0', sizeof(mytcpaddr));

    myudpaddr.sin_family = AF_INET;
    mytcpaddr.sin_family = AF_INET;
    myudpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mytcpaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    c_serveraddr.sin_family = AF_INET;
    hostptr=gethostbyname("tejo.tecnico.ulisboa.pt");
    // inet_aton("193.136.138.142",&ip);
    // c_serveraddr.sin_addr = ip;
    c_serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
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
        myip_arg=i+1;
        inet_aton(argv[myip_arg],&ip);
        arg_count++;
      }
      if(strcmp(argv[i],SERVER_UDP_PORT)==0)
      {
        myUDPport_arg=i+1;
        port=atoi(argv[i+1]);
        myudpaddr.sin_port = htons((u_short)port);
        arg_count++;
      }
      if(strcmp(argv[i],SERVER_TCP_PORT)==0)
      {
        myTCPport_arg=i+1;
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
    bind(clfd,(struct sockaddr*)&myudpaddr,sizeof(myudpaddr));
    printf("UDP: IP-%s, Port-%d\n",inet_ntoa(myudpaddr.sin_addr),myudpaddr.sin_port);
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
    printf("Selecting...\n");
    counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
    if (counter < 1)
      return 0;
    if (FD_ISSET(0,&rfds))
    {
      if(fgets(req,BUFFERSIZE,stdin)==NULL)
      printf("Couldn't read from terminal\n");
      else
      {
        sscanf(req, "%s %d\n", command, &service);
        printf("Service: %d\n", service);
        switch (status)
        {
          case idle:
          {
            if (strcmp(command,"join")==0)
            {
              if(service)
              {
                sprintf(msg,"GET_START %d;%d",service, myid);
                state=central_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  return 0;
                }
                i=sscanf(buffer,"OK %d;%d;%s", &id, &start_id, msg);
                if (i < 3 || id!=myid)
                {
                  printf("Trouble querying the central server\n");
                  return 0;
                }
                if (start_id)
                {
                  inet_aton(strtok(msg, ";"),&ip);
                  port=atoi(strtok(NULL, ";"));
                  printf("JOINING SERVER %s ON PORT %d\n", inet_ntoa(ip),port);
                  //TCP part
                }
                else
                {
                  //TCP part
                  sprintf(msg, "SET_START %d;%d;%s;%d", service, myid, inet_ntoa(mytcpaddr.sin_addr), atoi(argv[myTCPport_arg]));
                  state=central_contact(msg, c_serveraddr,fd,buffer);
                  if (state==SERV_TROUBLE)
                  {
                    printf("Trouble contacting the central server\n");
                    return 0;
                  }
                  i=sscanf(buffer, "OK %d;%*s\n", &id);
                  if (i < 1 || id!=myid)
                  {
                    printf("Trouble querying the central server\n");
                    return 0;
                  }
                  sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, inet_ntoa(myudpaddr.sin_addr), atoi(argv[myUDPport_arg]));
                  state=central_contact(msg, c_serveraddr,fd,buffer);
                  if (state==SERV_TROUBLE)
                  {
                    printf("Trouble contacting the central server\n");
                    return 0;
                  }
                  i=sscanf(buffer, "OK %d;%*s\n", &id);
                  if (i < 1 || id!=myid)
                  {
                    printf("Trouble querying the central server\n");
                    return 0;
                  }
                  status=on_ring;
                  break;
                }
              }
              else
              {
                printf("Invalid service id\n");
                break;
              }
            }
            else if (strcmp(command,"show_state")==0)
            {
              printf("TESTE\n");
              break;
            }
            else if (strcmp(command,"leave")==0)
            {
              printf("Not in a ring\n");
              break;
            }
            else if (strcmp(command,"exit")==0)
            {
              close (fd);
              return 1;
            }
            else
            {
              printf("Unrecognized command\n");
              break;
            }
          }
          case on_ring:
          {
            if (strcmp(command,"join")==0)
            {
              printf("Already in a ring. Leave first\n");
              break;
            }
            else if (strcmp(command,"show_state")==0)
            {
              printf("Disponível para serviço %d\n",service);
              break;
            }
            else if (strcmp(command,"leave")==0)
            {
              sprintf(msg, "WITHDRAW_DS %d;%d", service, myid);
              state=central_contact(msg, c_serveraddr,fd,buffer);
              if (state==SERV_TROUBLE)
              {
                printf("Trouble contacting the central server\n");
                return 0;
              }
              i=sscanf(buffer, "OK %d;%*s\n", &id);
              if (i < 1 || id!=myid)
              {
                printf("Trouble querying the central server\n");
                return 0;
              }
              sprintf(msg, "WITHDRAW_START %d;%d", service, myid);
              state=central_contact(msg, c_serveraddr,fd,buffer);
              if (state==SERV_TROUBLE)
              {
                printf("Trouble contacting the central server\n");
                return 0;
              }
              i=sscanf(buffer, "OK %d;%*s\n", &id);
              if (i < 1 || id!=myid)
              {
                printf("Trouble querying the central server\n");
                return 0;
              }
              status=idle;
              break;
            }
            else if (strcmp(command,"exit")==0)
            {
              printf("Still in a ring. Leave first\n");
              break;
            }
          }
          case busy:
          {
            if (strcmp(command,"join")==0)
            {
              printf("Already in a ring. Leave first\n");
              break;
            }
            else if (strcmp(command,"leave")==0)
            {
              printf("Finish servicing client\n");
              break;
            }
            else if (strcmp(command,"show_state")==0)
            {
              printf("Unavailable\n");
              break;
            }
            else if (strcmp(command,"exit")==0)
            {
              close (fd);
              return 1;
            }
          }
        }
      }
    }
    if (FD_ISSET(clfd,&rfds))
    {
      switch (state)
      {
        case busy:
        {
          printf("GOT HERE\n");
          addrlen = sizeof(clientaddr);
          recv_bytes=recvfrom(clfd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&clientaddr, &addrlen);
          if (recv_bytes == -1)
          {
            perror("Error: ");
            return 0;
          }
          if (strcmp(req,LEAVING_DISPATCH)==0)
          {
            sprintf(msg, LEFT_DISPATCH);
            sent_bytes=sendto(clfd, msg, strlen(msg), 0, (struct sockaddr*)&clientaddr, addrlen);
            if (sent_bytes == -1)
            {
              perror("Error: ");
              return 0;
            }
            sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, inet_ntoa(myudpaddr.sin_addr), atoi(argv[myUDPport_arg]));
            state=central_contact(msg, c_serveraddr,fd,buffer);
            if (state==SERV_TROUBLE)
            {
              printf("Trouble contacting the central server\n");
              return 0;
            }
            i=sscanf(buffer, "OK %d;%*s\n", &id);
            if (i < 1 || id!=myid)
            {
              printf("Trouble querying the central server\n");
              return 0;
            }
            status=on_ring;
            break;
          }
        }
        case on_ring:
        {
          memset(buffer,'\0',BUFFERSIZE);
          recv_bytes=recvfrom(clfd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&clientaddr, &addrlen);
          if (recv_bytes < 0)
          {
            perror("Error: ");
            return 0;
          }
          printf("RECEIVED: %s\n",buffer);
          if (strcmp(buffer,JOINING_DISPATCH)==0)
          {
            sprintf(msg, JOINED_DISPATCH);
            sent_bytes=sendto(clfd, msg, strlen(msg), 0, (struct sockaddr*)&clientaddr, addrlen);
            if (sent_bytes == -1)
            {
              perror("Error: ");
              return 0;
            }
            sprintf(msg, "WITHDRAW_DS %d;%d", service, myid);
            state=central_contact(msg, c_serveraddr,fd,buffer);
            if (state==SERV_TROUBLE)
            {
              printf("Trouble contacting the central server\n");
              return 0;
            }
            i=sscanf(buffer, "OK %d;%*s\n", &id);
            if (i < 1 || id!=myid)
            {
              printf("Trouble querying the central server\n");
              return 0;
            }
            state=busy;
            break;
          }
        }
      }
    }
  }
}
