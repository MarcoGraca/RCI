#include "contacts.h"

int main(int argc, char *argv[])
{
  int i, recv_bytes, sent_bytes, port, arg_count = 0, myid, start_id, id, service, state, myip_arg, myTCPport_arg, myUDPport_arg, counter, maxfd;
  int fd, clfd, prev_fd, next_fd, vol_fd, new_fd, ring_state, next_id=0, isstart = 0, isdispatch = 0, multi_prev = 0;
  fd_set rfds;
  socklen_t addrlen;
  struct sockaddr_in c_serveraddr, myudpaddr, mytcpaddr, clientaddr, n_serveraddr, p_serveraddr;
  struct in_addr ip;
  struct hostent *hostptr;
  char msg[BUFFERSIZE], buffer[BUFFERSIZE], req[BUFFERSIZE], command[BUFFERSIZE], token;
  enum {idle, on_ring, busy} status;
  fd = socket(AF_INET,SOCK_DGRAM,0);
  clfd = socket(AF_INET,SOCK_DGRAM,0);
  prev_fd = socket(AF_INET,SOCK_STREAM,0);
  next_fd = socket(AF_INET,SOCK_STREAM,0);
  vol_fd = socket(AF_INET,SOCK_STREAM,0);
  new_fd = socket(AF_INET,SOCK_STREAM,0);
  if (argc > 8 && argc % 2)
  {
    memset((void*)&c_serveraddr,(int)'\0', sizeof(c_serveraddr));
    memset((void*)&myudpaddr,(int)'\0', sizeof(myudpaddr));
    memset((void*)&mytcpaddr,(int)'\0', sizeof(mytcpaddr));
    c_serveraddr.sin_family = AF_INET;
    n_serveraddr.sin_family = AF_INET;
    myudpaddr.sin_family = AF_INET;
    mytcpaddr.sin_family = AF_INET;
    n_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

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
        myudpaddr.sin_addr=ip;
        mytcpaddr.sin_addr=ip;
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
    if(bind(clfd,(struct sockaddr*)&myudpaddr,sizeof(myudpaddr))==-1)
    {
      perror("UDP Bind Error ");
      exit (0);
    }
    if(bind(prev_fd,(struct sockaddr*)&mytcpaddr,sizeof(mytcpaddr))==-1)
    {
      perror("TCP Bind Error ");
      exit (0);
    }
    listen(prev_fd,5);
    if (arg_count < 3)
    {
      printf("Wrong invocation\n");
      exit(0);
    }
  }
  else
  {
    printf("Wrong invocation\n");
    exit(0);
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
      FD_SET(prev_fd,&rfds);
      maxfd=max(clfd,prev_fd);
      if (multi_prev)
      {
        FD_SET(new_fd,&rfds);
        maxfd=max(maxfd,new_fd);
        if (multi_prev > 1)
        {
          FD_SET(vol_fd,&rfds);
          maxfd=max(maxfd,vol_fd);
        }
      }
    }
    printf("Selecting...\n");
    counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
    if (counter < 1)
      exit(0);
    if (FD_ISSET(0,&rfds))
    {
      if(fgets(req,BUFFERSIZE,stdin)==NULL)
      printf("Couldn't read from terminal\n");
      else
      {
        sscanf(req, "%s %d\n", command, &service);
        switch (status)
        {
          case idle:
          {
            if (strcmp(command,"join")==0)
            {
              if(service)
              {
                sprintf(msg,"GET_START %d;%d",service, myid);
                state=UDP_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  exit(0);
                }
                i=sscanf(buffer,"OK %d;%d;%s", &id, &start_id, msg);
                if (i < 3 || id!=myid)
                {
                  printf("Trouble decoding the central server\n");
                  exit(0);
                }
                if (start_id)
                {
                  inet_aton(strtok(msg, ";"),&ip);
                  port=atoi(strtok(NULL, ";"));
                  printf("JOINING SERVER %s ON PORT %d\n", inet_ntoa(ip),port);
                  //TCP part
                  n_serveraddr.sin_family = AF_INET;
                  n_serveraddr.sin_addr = ip;
                  n_serveraddr.sin_port = htons((u_short)port);
                  if(connect(next_fd,(struct sockaddr*)&n_serveraddr,sizeof(n_serveraddr))==-1)
                  {
                    perror("Connect Error ");
                    exit(0);
                  }
                  printf("CONNECTED TO SERVER %d\n", start_id);
                  next_id=start_id;
                  sprintf(msg, "NEW %d;%s;%d\n", myid, inet_ntoa(mytcpaddr.sin_addr), atoi(argv[myTCPport_arg]));
                  TCP_write(next_fd,msg);
                  status=on_ring;
                }
                else
                {
                  sprintf(msg, "SET_START %d;%d;%s;%d", service, myid, inet_ntoa(mytcpaddr.sin_addr), atoi(argv[myTCPport_arg]));
                  state=UDP_contact(msg, c_serveraddr,fd,buffer);
                  if (state==SERV_TROUBLE)
                  {
                    printf("Trouble contacting the central server\n");
                    exit(0);
                  }
                  i=sscanf(buffer, "OK %d;%*s", &id);
                  if (i < 1 || id!=myid)
                  {
                    printf("Trouble decoding the central server\n");
                    exit(0);
                  }
                  isstart = 1;
                  sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, inet_ntoa(myudpaddr.sin_addr), atoi(argv[myUDPport_arg]));
                  state=UDP_contact(msg, c_serveraddr,fd,buffer);
                  if (state==SERV_TROUBLE)
                  {
                    printf("Trouble contacting the central server\n");
                    exit(0);
                  }
                  i=sscanf(buffer, "OK %d;%*s", &id);
                  if (i < 1 || id!=myid)
                  {
                    printf("Trouble decoding the central server\n");
                    exit(0);
                  }
                  isdispatch = 1;
                  status=on_ring;
                  ring_state=RING_FREE;
                }
              }
              else
                printf("Invalid service id\n");
            }
            else if (strcmp(command,"show_state")==0)
              printf("Not in a ring\n");
            else if (strcmp(command,"leave")==0)
              printf("Not in a ring\n");
            else if (strcmp(command,"exit")==0)
            {
              close (fd);
              close (prev_fd);
              exit(1);
            }
            else
              printf("Unrecognized command\n");
            break;
          }
          case on_ring:
          {
            if (strcmp(command,"join")==0)
              printf("Already in a ring. Leave first\n");
            else if (strcmp(command,"show_state")==0)
              printf("Available for service: %d; Ring available; Successor ID:%d\n", service, next_id);
            else if (strcmp(command,"leave")==0)
            {
              if (!next_id)
              {
                sprintf(msg, "WITHDRAW_DS %d;%d", service, myid);
                state=UDP_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  exit(0);
                }
                i=sscanf(buffer, "OK %d;%*s", &id);
                if (i < 1 || id!=myid)
                {
                  printf("Trouble decoding the central server\n");
                  exit(0);
                }
                isdispatch = 0;
                sprintf(msg, "WITHDRAW_START %d;%d", service, myid);
                state=UDP_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  exit(0);
                }
                i=sscanf(buffer, "OK %d;%*s", &id);
                if (i < 1 || id!=myid)
                {
                  printf("Trouble decoding the central server\n");
                  exit(0);
                }
                isstart = 0;
                status=idle;
              }
              else
              {
                sprintf(msg, "TOKEN %d;%c;%d;%s;%d\n", myid, LEFT, next_id, inet_ntoa(mytcpaddr.sin_addr), atoi(argv[myTCPport_arg]));
                TCP_write(next_fd,msg);
              }
            }
            else if (strcmp(command,"exit")==0)
              printf("Still in a ring. Leave first\n");
            else
              printf("Unrecognized command\n");
            break;
          }
          case busy:
          {
            if (strcmp(command,"join")==0)
              printf("Already in a ring. Leave first\n");
            else if (strcmp(command,"leave")==0)
              printf("Finish servicing client\n");
            else if (strcmp(command,"show_state")==0)
            {
              if (ring_state)
                printf("Unavailable for service: %d; Ring available; Successor ID:%d\n", service, next_id);
              else
                printf("Unavailable for service: %d; Ring unavailable; Successor ID:%d\n", service, next_id);
            }
            else if (strcmp(command,"exit")==0)
              printf("Finish servicing client\n");
            else
              printf("Unrecognized command\n");
            break;
          }
        }
      }
    }
    if (FD_ISSET(clfd,&rfds))
    {
      switch (status)
      {
        case idle:
          break;
        case busy:
        {
          memset(buffer,'\0',BUFFERSIZE);
          addrlen = sizeof(clientaddr);
          recv_bytes=recvfrom(clfd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&clientaddr, &addrlen);
          if (recv_bytes == -1)
          {
            perror("UDP recv Error ");
            exit(0);
          }
          printf("RECEIVED: %s\n",buffer);
          if (strcmp(buffer,LEAVING_DISPATCH)==0)
          {
            sprintf(msg, LEFT_DISPATCH);
            sent_bytes=sendto(clfd, msg, strlen(msg), 0, (struct sockaddr*)&clientaddr, addrlen);
            if (sent_bytes == -1)
            {
              perror("UDP send Error ");
              exit(0);
            }
            status=on_ring;
            if (!next_id)
            {
              sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, inet_ntoa(myudpaddr.sin_addr), atoi(argv[myUDPport_arg]));
              state=UDP_contact(msg, c_serveraddr,fd,buffer);
              if (state==SERV_TROUBLE)
              {
                printf("Trouble contacting the central server\n");
                exit(0);
              }
              i=sscanf(buffer, "OK %d;%*s", &id);
              if (i < 1 || id!=myid)
              {
                printf("Trouble decoding the central server\n");
                exit(0);
              }
              isdispatch = 1;
            }
            else
            {
              sprintf(msg, "TOKEN %d;%c\n", myid, AVLB);
              TCP_write(next_fd,msg);
            }
          }
          break;
        }
        case on_ring:
        {
          memset(buffer,'\0',BUFFERSIZE);
          recv_bytes=recvfrom(clfd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&clientaddr, &addrlen);
          if (recv_bytes < 0)
          {
            perror("UDP recv Error ");
            exit(0);
          }
          printf("RECEIVED: %s\n",buffer);
          if (strcmp(buffer,JOINING_DISPATCH)==0)
          {
            sprintf(msg, JOINED_DISPATCH);
            sent_bytes=sendto(clfd, msg, strlen(msg), 0, (struct sockaddr*)&clientaddr, addrlen);
            if (sent_bytes == -1)
            {
              perror("UDP send Error ");
              exit(0);
            }
            sprintf(msg, "WITHDRAW_DS %d;%d", service, myid);
            state=UDP_contact(msg, c_serveraddr,fd,buffer);
            if (state==SERV_TROUBLE)
            {
              printf("Trouble contacting the central server\n");
              exit(0);
            }
            i=sscanf(buffer, "OK %d;%*s", &id);
            if (i < 1 || id!=myid)
            {
              printf("Trouble decoding the central server\n");
              exit(0);
            }
            isdispatch = 0;
            status=busy;
            if (next_id)
            {
              sprintf(msg, "TOKEN %d;%c\n", myid, SRC_D);
              TCP_write(next_fd,msg);
            }
            else
              ring_state=RING_BUSY;
          }
          break;
        }
      }
    }
    if (FD_ISSET(prev_fd,&rfds))
    {
      printf("Got an accept\n");
      addrlen=sizeof(p_serveraddr);
      if ((vol_fd=accept(prev_fd,(struct sockaddr*)&p_serveraddr, &addrlen))==-1)
      {
        perror("TCP Accept Error ");
        exit(0);
      }
      if (multi_prev == 0)
        new_fd=vol_fd;
      multi_prev++;
    }
    memset(msg,'\0',BUFFERSIZE);
    if (FD_ISSET(vol_fd,&rfds))
    {
      printf("Got message for temporary\n");
      state=TCP_read(vol_fd,msg);
      if (state == SERV_TROUBLE)
        exit(0);
    }
    if (FD_ISSET(new_fd,&rfds))
    {
      printf("Got message for regular\n");
      state=TCP_read(new_fd,msg);
      if (state == SERV_TROUBLE)
      {
        if (multi_prev)
        {
          close (new_fd);
          new_fd=vol_fd;
          multi_prev--;
          printf("Closed connection to old predecessor: %d\n", multi_prev);
        }
        else
          exit(0);
      }
    }
    if (strlen(msg))
    {
      memset(req,'\0',BUFFERSIZE);
      memset(buffer,'\0',BUFFERSIZE);
      i=sscanf(msg,"%s %s\n",req, buffer);
      if (i < 1 || i > 2)
        printf("Trouble decoding what the ring sent\n");
      else if (i == 2)
      {
        if (strcmp(req,"TOKEN")==0)
        {
          memset(req,'\0',BUFFERSIZE);
          i=sscanf(buffer, "%d;%c;%s", &start_id, &token, req);
          if (id == 0 || i < 2)
            printf("Trouble decoding what the ring sent\n");
          else if (i == 2)
          {
            switch (token)
            {
              case SRC_D:
              {
                if (status == busy && next_id)
                {
                  if (start_id != myid)
                    TCP_write(next_fd,msg);
                  else
                  {
                    sprintf(msg, "TOKEN %d;%c\n", myid, UNAV);
                    TCP_write(next_fd,msg);
                  }
                }
                else if (status == on_ring)
                {
                  sprintf(req, "SET_DS %d;%d;%s;%d", service, myid, inet_ntoa(myudpaddr.sin_addr), atoi(argv[myUDPport_arg]));
                  state=UDP_contact(req, c_serveraddr,fd,buffer);
                  if (state==SERV_TROUBLE)
                  {
                    printf("Trouble contacting the central server\n");
                    exit(0);
                  }
                  i=sscanf(buffer, "OK %d;%*s", &id);
                  if (i < 1 || id!=myid)
                  {
                    printf("Trouble decoding the central server\n");
                    exit(0);
                  }
                  isdispatch = 1;
                  if (next_id)
                  {
                    sprintf(msg, "TOKEN %d;%c\n", myid, FND_D);
                    TCP_write(next_fd,msg);
                  }
                }
                else
                  ring_state=RING_BUSY;
                break;
              }
              case FND_D:
              {
                if (start_id!=myid)
                  TCP_write(next_fd,msg);
                ring_state=RING_FREE;
                break;
              }
              case AVLB:
              {
                if(status==busy || start_id < myid)
                {
                  TCP_write(next_fd,msg);
                  ring_state=RING_FREE;
                }
                else if(start_id == myid)
                {
                  sprintf(msg, "SET_DS %d;%d;%s;%d", service, myid, inet_ntoa(myudpaddr.sin_addr), atoi(argv[myUDPport_arg]));
                  state=UDP_contact(msg, c_serveraddr,fd,buffer);
                  if (state==SERV_TROUBLE)
                  {
                    printf("Trouble contacting the central server\n");
                    exit(0);
                  }
                  i=sscanf(buffer, "OK %d;%*s", &id);
                  if (i < 1 || id!=myid)
                  {
                    printf("Trouble decoding the central server\n");
                    exit(0);
                  }
                  isdispatch = 1;
                }
                else
                {
                  sprintf(msg, "TOKEN %d;%c\n", myid, AVLB);
                  TCP_write(next_fd,msg);
                }
                break;
              }
              case UNAV:
              {
                if(status == busy)
                {
                  ring_state=RING_BUSY;
                  if(start_id!=myid)
                    TCP_write(next_fd,msg);
                }
                else
                {
                  sprintf(msg, "TOKEN %d;%c\n", myid, AVLB);
                  TCP_write(next_fd,msg);
                }
                break;
              }
              default:
              {
                printf("Trouble decoding what token the ring sent\n");
                break;
              }
            }
          }
          else
          {
            memset(buffer,'\0',BUFFERSIZE);
            i=sscanf(req, "%d;%s", &id, buffer);
            if (i < 2 || (token != JOINED && token != LEFT)) //Maybe more statements here & attention to limit cases of 2 servers on ring
              printf("Trouble decoding what the ring sent\n");
            else if (start_id == next_id)
            {
              if (id == myid && token == LEFT) //2 servers on ring
              {
                TCP_write(next_fd,msg);
                close(next_fd);
                next_id=0;
                next_fd=socket(AF_INET,SOCK_STREAM,0);
              }
              else
              {
                inet_aton(strtok(buffer, ";"),&ip);
                port=atoi(strtok(NULL, ";"));
                printf("JOINING SERVER %s ON PORT %d\n", inet_ntoa(ip),port);
                n_serveraddr.sin_family = AF_INET;
                n_serveraddr.sin_addr = ip;
                n_serveraddr.sin_port = htons((u_short)port);
                if (token == LEFT)
                  TCP_write(next_fd,msg);
                close(next_fd);
                next_fd=socket(AF_INET,SOCK_STREAM,0);
                if(connect(next_fd,(struct sockaddr*)&n_serveraddr,sizeof(n_serveraddr))==-1)
                {
                  perror("Connect Error ");
                  exit(0);
                }
                printf("CONNECTED TO SERVER %d\n", id);
                next_id=id;
                if (ring_state == RING_BUSY && token == JOINED)
                {
                  sprintf(msg, "TOKEN %d;%c\n", myid, UNAV);
                  TCP_write(next_fd,msg);
                }
              }
            }
            else if (start_id == myid && token == LEFT)
            {
              if (isdispatch)
              {
                sprintf(msg, "WITHDRAW_DS %d;%d", service, myid);
                state=UDP_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  exit(0);
                }
                i=sscanf(buffer, "OK %d;%*s", &id);
                if (i < 1 || id!=myid)
                {
                  printf("Trouble decoding the central server\n");
                  exit(0);
                }
                isdispatch = 0;
                sprintf(msg, "TOKEN %d;%c\n", myid, SRC_D);
                TCP_write(next_fd,msg);
              }
              if (isstart)
              {
                sprintf(msg, "WITHDRAW_START %d;%d", service, myid);
                state=UDP_contact(msg, c_serveraddr,fd,buffer);
                if (state==SERV_TROUBLE)
                {
                  printf("Trouble contacting the central server\n");
                  exit(0);
                }
                i=sscanf(buffer, "OK %d;%*s", &id);
                if (i < 1 || id!=myid)
                {
                  printf("Trouble decoding the central server\n");
                  exit(0);
                }
                isstart = 0;
                sprintf(msg, "NEW_START\n");
                TCP_write(next_fd,msg);
              }
              close(next_fd);
              close(new_fd);
              multi_prev--;
              next_fd = socket(AF_INET,SOCK_STREAM,0);
              next_id=0;
              status=idle;
            }
            else
              TCP_write(next_fd,msg);
          }
        }
        else if (strcmp(req,"NEW")==0)
        {
          memset(req,'\0',BUFFERSIZE);
          i=sscanf(buffer,"%d;%s", &id, req);
          if(id == 0 || i < 2)
            printf("Trouble decoding what the new server sent\n");
          else if (id == myid)
            printf("Trying to declare a new server with an used id\n");
          else
          {
            if (!next_id)
            {
              inet_aton(strtok(req, ";"),&ip);
              port=atoi(strtok(NULL, ";"));
              n_serveraddr.sin_addr=ip;
              n_serveraddr.sin_port=htons((u_short)port);
              if(connect(next_fd,(struct sockaddr*)&n_serveraddr,sizeof(n_serveraddr))==-1)
              {
                perror("Connect Error ");
                exit(0);
              }
              printf("CONNECTED TO SERVER %d\n", id);
              next_id=id;
            }
            else
            {
              sprintf(msg, "TOKEN %d;N;%s\n", myid, buffer);
              TCP_write(next_fd, msg);
            }
          }
        }
        else
          printf("Trouble decoding what the ring sent\n");
      }
      else
      {
        if (strcmp(req,"NEW_START")==0)
        {
          sprintf(msg, "SET_START %d;%d;%s;%d", service, myid, inet_ntoa(mytcpaddr.sin_addr), atoi(argv[myTCPport_arg]));
          state=UDP_contact(msg, c_serveraddr,fd,buffer);
          if (state==SERV_TROUBLE)
          {
            printf("Trouble contacting the central server\n");
            exit(0);
          }
          i=sscanf(buffer, "OK %d;%*s", &id);
          if (i < 1 || id!=myid)
          {
            printf("Trouble decoding the central server\n");
            exit(0);
          }
          isstart=1;
        }
        else
          printf("Trouble decoding what the ring sent\n");
      }
    }
  }
}
