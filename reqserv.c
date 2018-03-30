#include "contacts.h"

int main(int argc, char* argv[])
{
  char req[50], command[25], msg[50], buffer[50];
  struct hostent *hostptr;
  struct in_addr dsip;
  int fd = socket(AF_INET,SOCK_DGRAM,0), dsfd = socket(AF_INET,SOCK_DGRAM,0), service=0, port=PORT, dsid, dspt, state, i;
  enum {idle, busy} status;
  struct sockaddr_in c_serveraddr, d_serveraddr;
  struct in_addr ip;
  hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");
  for (i=1; i<argc; i+=2)
  {
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
  memset((void*)&c_serveraddr,(int)'\0', sizeof(c_serveraddr));
  c_serveraddr.sin_family = AF_INET;
  c_serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
  c_serveraddr.sin_port = htons((u_short)port);
  status=idle;
  while(1)
  {
    if (fgets(req, BUFFERSIZE, stdin)!=NULL)
    {
      i=sscanf(req, "%s %d\n", command, &service);
      switch (status)
      {
        case idle:
        {
          if (strcmp(command,"request_service")==0||strcmp(command,"rs")==0)
          {
            if (service)
            {
              sprintf(msg, "GET_DS_SERVER %d",service);
              state=UDP_contact(msg, c_serveraddr,fd,buffer);
              if (state==SERV_TROUBLE)
              {
                printf("Trouble contacting the central server\n");
                exit(0);
              }
              i=sscanf(buffer, "OK %d;%s", &dsid, msg);
              if (i < 2)
              {
                printf("Trouble querying the central server\n");
                exit(0);
              }
              inet_aton(strtok(msg, ";"),&dsip);
              dspt=atoi(strtok(NULL,";"));
              memset((void*)&d_serveraddr,(int)'\0', sizeof(d_serveraddr));
              d_serveraddr.sin_family = AF_INET;
              d_serveraddr.sin_addr = dsip;
              d_serveraddr.sin_port = htons((u_short)dspt);
              printf("IP-%s Port-%d\n",inet_ntoa(d_serveraddr.sin_addr),d_serveraddr.sin_port);
              state=UDP_contact(JOINING_DISPATCH, d_serveraddr,dsfd,buffer);
              if (state==SERV_TROUBLE)
              {
                printf("Trouble contacting the dispatch server\n");
                exit(0);
              }
              if (strcmp(buffer,JOINED_DISPATCH)==0)
              {
                printf("Service successfully initialized\n");
                status=busy;
              }
            }
            else
              printf("Invalid service id\n");
            break;
          }
          else if(strcmp(command,"terminate_service")==0||strcmp(command,"ts")==0)
          {
            printf("Not requesting any service at the moment\n");
            break;
          }
          else if(strcmp(command,"exit")==0)
          {
            close(fd);
            exit(1);
          }
          else
          {
            printf("Unrecognized command\n");
            break;
          }
        }
        case busy:
        {
          if (strcmp(command,"request_service")==0||strcmp(command,"rs")==0)
          {
            printf("GOT HERE INSTEAD\n");
            printf("Terminate the current provided service first\n");
            break;
          }
          else if(strcmp(command,"terminate_service")==0||strcmp(command,"ts")==0)
          {
            printf("GOT HERE\n");
            memset(buffer,'\0',BUFFERSIZE);
            state=UDP_contact(LEAVING_DISPATCH, d_serveraddr,dsfd,buffer);
            if (state==SERV_TROUBLE)
            {
              printf("Trouble contacting the dispatch server\n");
              exit(0);
            }
            if (strcmp(buffer,LEFT_DISPATCH)==0)
            {
              printf("Service successfully terminated\n");
              status=idle;
            }
            break;
          }
          else if(strcmp(command,"exit")==0)
          {
            printf("Terminate the current provided service first\n");
            break;
          }
          else
          {
            printf("Unrecognized command\n");
            break;
          }
        }
      }
    }
  }
}
