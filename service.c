#include "contacts.h"

int main(int argc, char *argv[])
{
  int i, recv_bytes, sent_bytes, port, arg_count = 0, myid, start_id, id, service, state, myip_arg, myTCPport_arg, myUDPport_arg, counter, maxfd;
  int fd, clfd, prev_fd, next_fd, vol_fd, new_fd, ring_state=RING_FREE, next_id=0, isstart = 0, isdispatch = 0, multi_prev = 0, isleaving = 0;
  fd_set rfds;
  socklen_t addrlen;
  struct sockaddr_in c_serveraddr, myudpaddr, mytcpaddr, clientaddr, n_serveraddr, p_serveraddr;
  struct in_addr ip;
  struct hostent *hostptr;
  char msg[BUFFERSIZE], buffer[BUFFERSIZE], req[BUFFERSIZE], command[BUFFERSIZE], token;
  //3-states application
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
    //cycle to process the additional bash info
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
    counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
    if (counter < 1)
      exit(0);
    //Is able to read message from terminal
    if (FD_ISSET(0,&rfds))
    {
      if(fgets(req,BUFFERSIZE,stdin)==NULL)
      printf("Couldn't read from terminal\n");
      else
      {
        sscanf(req, "%s %d\n", command, &service);
        switch (status)
        {
          //Neither providing a service nor in a ring or registered on the central server
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
                //There is already a server providing that service. Proceed to join its ring
                if (start_id)
                {
                  inet_aton(strtok(msg, ";"),&ip);
                  port=atoi(strtok(NULL, ";"));
                  printf("JOINING SERVER %s ON PORT %d\n", inet_ntoa(ip),port);
                  n_serveraddr.sin_family = AF_INET;
                  n_serveraddr.sin_addr = ip;
                  n_serveraddr.sin_port = htons((u_short)port);
                  //Connecting with his TCP port
                  if(connect(next_fd,(struct sockaddr*)&n_serveraddr,sizeof(n_serveraddr))==-1)
                  {
                    perror("Connect Error ");
                    exit(0);
                  }
                  printf("CONNECTED TO SERVER %d\n", start_id);
                  next_id=start_id;
                  sprintf(msg, "NEW %d;%s;%d\n", myid, inet_ntoa(mytcpaddr.sin_addr), atoi(argv[myTCPport_arg]));
                  //Welcoming the new server
                  TCP_write(next_fd,msg);
                  status=on_ring;
                }
                //No server providing it. Proceeding to register at the central server and forming a ring
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
          //On a ring of servers that provide a certain service but not connected to a client at the moment
          case on_ring:
          {
            if (strcmp(command,"join")==0)
              printf("Already in a ring. Leave first\n");
            else if (strcmp(command,"show_state")==0)
              {
                if(next_id)
                  printf("Available for service: %d; Ring available; Successor ID:%d\n", service, next_id);
                else
                  printf("Available for service: %d; Ring available; Alone\n", service);
              }

            else if (strcmp(command,"leave")==0)
            {
              //Alone in the ring
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
                //Currently the start server
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
                //Currently the dispatch server
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
                  //The dispatch server must first ensure there is another server to dispatch the service or
                  // if not, that the other servers in the ring know the ring state after its departure
                  sprintf(msg, "TOKEN %d;%c\n", myid, SRC_D);
                }
                else
                  //With no need to update the ring state, the server will just announce it's departure to the ring
                  sprintf(msg, "TOKEN %d;%c;%d;%s;%d\n", myid, LEFT, next_id, inet_ntoa(n_serveraddr.sin_addr), ntohs(n_serveraddr.sin_port));
                TCP_write(next_fd,msg);
                isleaving=1;
              }
            }
            else if (strcmp(command,"exit")==0)
              printf("Still in a ring. Leave first\n");
            else
              printf("Unrecognized command\n");
            break;
          }
          //In a ring but currently occupied providing a service
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
              {
                if(next_id)
                  printf("Unavailable for service: %d; Ring unavailable; Successor ID:%d\n", service, next_id);
                else
                  printf("Unavailable for service: %d; Ring unavailable; Alone\n", service);
              }

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
    //Able to read message from the server's UDP port
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
          //A client announces it no longer needs the provided service
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
            //Alone on the ring, automatically registers itself on the central server as the dispatch server
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
              //Communicate to the rest of the ring that one of its servers is still available to provide its service
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
          //A client is requesting the ring's service
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
              //Search the ring for a new server to register as dispatch
              TCP_write(next_fd,msg);
            }
            else
              //No other servers in the ring. The ring is occupied by default
              ring_state=RING_BUSY;
          }
          break;
        }
      }
    }
    //A new server is contacting the ring or a different one than its predecessor
    if (FD_ISSET(prev_fd,&rfds))
    {
      addrlen=sizeof(p_serveraddr);
      //Store the new file descriptor as a temporary one just while the new ring is forming
      if ((vol_fd=accept(prev_fd,(struct sockaddr*)&p_serveraddr, &addrlen))==-1)
      {
        perror("TCP Accept Error ");
        exit(0);
      }
      if (multi_prev == 0)
        //This server was alone in the ring so the temporary file descriptor becomes the permanent as there was no previous predecessor
        new_fd=vol_fd;
      //Register the server predecessors' number increase
      multi_prev++;
    }
    memset(msg,'\0',BUFFERSIZE);
    //Able to read from the temporary socket meant for servers still in the process of joining the ring
    if (FD_ISSET(vol_fd,&rfds)&& multi_prev == 2)
    {
      printf("Got message for temporary socket\n");
      state=TCP_read(vol_fd,msg);
      if (state == SERV_TROUBLE)
        exit(0);
    }
    //Able to read from its predecessor in the ring
    if (FD_ISSET(new_fd,&rfds))
    {
      state=TCP_read(new_fd,msg);
      if (state == SERV_TROUBLE)
      {
        //This server's previous predecessor notified us that its connection is now closed
        //due to either its departure from the ring or the addition of another between them
        if (multi_prev)
        {
          close(new_fd);
          //The temporary socket now assumes a permanent position,
          new_fd=vol_fd;
          //Register the server predecessors' number decrease
          multi_prev--;
          printf("Closed connection to an old predecessor\n");
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
              //The servers in the ring are currently searching for a new dispatch server
              case SRC_D:
              {
                //The server is occupied or preparing to leave
                if ((status == busy && next_id)||isleaving)
                {
                  if (start_id != myid)
                    //Repass the message to its sucessor in the ring
                    TCP_write(next_fd,msg);
                  else
                  {
                    sprintf(msg, "TOKEN %d;%c\n", myid, UNAV);
                    //This server sent the token, so no other servers in the ring are available to be dispatch servers.
                    //Share that the ring is unavailable with the next server
                    TCP_write(next_fd,msg);
                  }
                }
                //This server is available to become the dispatch server
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
                    //Share the ring's dispatch server redefinition with the other servers
                    TCP_write(next_fd,msg);
                  }
                }
                else
                  //No other servers. Ring becomes unavailable by default
                  ring_state=RING_BUSY;
                break;
              }
              //A server in the ring became the new dispatch server
              case FND_D:
              {
                if (start_id!=myid)
                  TCP_write(next_fd,msg);
                if (isleaving)
                {
                  sprintf(msg, "TOKEN %d;%c;%d;%s;%d\n", myid, LEFT, next_id, inet_ntoa(n_serveraddr.sin_addr), ntohs(n_serveraddr.sin_port));
                  //With the guarantee that there is a new server to dispatch information proceed to leave the ring
                  TCP_write(next_fd,msg);
                }
                ring_state=RING_FREE;
                break;
              }
              //A server in the ring finished servicing a client, so the ring is available again
              case AVLB:
              {
                ring_state=RING_FREE;
                //This server either has a superior ID than the one that sent this message or is currently busy
                //So it just repasses the message
                if(status==busy || start_id < myid)
                  TCP_write(next_fd,msg);
                //This was the server that sent this token so with the guarantee that it is the one with the lowest ID
                //In the ring, it registers itself as the new dispatch server
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
                  //This server has an inferior ID than the one that sent this message so it stops the circulating token and
                  //Passes one with its ID to the next server in the ring proposing itself instead as the new dispatch server
                  TCP_write(next_fd,msg);
                }
                break;
              }
              //A server tried to find a new dispatch server, failed to do so and is now sharing the ring's occupied state with the others
              case UNAV:
              {
                if(status == busy || isleaving)
                {
                  ring_state=RING_BUSY;
                  if(start_id!=myid)
                    TCP_write(next_fd,msg);
                  //This server was leaving the ring and is now sure the others in the ring know its state due to its departure
                  //So it proceeds with the departure process
                  else if(isleaving)
                  {
                    sprintf(msg, "TOKEN %d;%c;%d;%s;%d\n", myid, LEFT, next_id, inet_ntoa(n_serveraddr.sin_addr), ntohs(n_serveraddr.sin_port));
                    TCP_write(next_fd,msg);
                  }
                }
                else
                {
                  sprintf(msg, "TOKEN %d;%c\n", myid, AVLB);
                  //This server became available since the last query for a dispatch server in the ring so it will now share
                  //The change in the ring's state with the other servers
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
            //Either a server is notifying the ring of its departure or the starting server is announcing a new member of the ring
            if (i < 2 || (token != JOINED && token != LEFT))
              printf("Trouble decoding what the ring sent\n");
            //This server's sucessor is the one who sent this token
            else if (start_id == next_id)
            {
              //There are 2 servers on the ring, so the token's information about its sender's sucessor is redundant
              if (id == myid && token == LEFT)
              {
                TCP_write(next_fd,msg);
                close(next_fd);
                //This server notes it's alone in the ring
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
                  //Send the departure token back to its sender to assure it it's safe to leave the ring without it breaking
                  TCP_write(next_fd,msg);
                //Close the connection with the successor, due to either its departure or the addition of another between them
                close(next_fd);
                next_fd=socket(AF_INET,SOCK_STREAM,0);
                //Connect to the new sucessor
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
                  //Share the ring's unavailable state with the new server
                  TCP_write(next_fd,msg);
                }
              }
            }
            else if (start_id == myid && token == LEFT)
            {
              close(next_fd);
              close(new_fd);
              multi_prev--;
              next_fd = socket(AF_INET,SOCK_STREAM,0);
              next_id=0;
              //The departure process is finished
              isleaving=0;
              status=idle;
            }
            else
              TCP_write(next_fd,msg);
          }
        }
        //A server informed the dispatch of its intention to join the ring
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
            //With no other server than the starting one on the ring, no token is passed and the connection is immediate
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
              sprintf(msg, "TOKEN %d;%c;%s\n", myid, JOINED, buffer);
              //Announce the new member of the ring to the other servers
              TCP_write(next_fd, msg);
            }
          }
        }
        else
          printf("Trouble decoding what the ring sent\n");
      }
      else
      {
        //The starting server is leaving the ring so it informs this server, its sucessor, that it'll become the new starting server
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
