#include "contacts.h"

void TCP_write(int afd, char *msg)
{
  int n_bytes, n_left, n_done;
  char *ptr;
  n_bytes=strlen(msg);
  ptr=msg;
  n_left=n_bytes;
  while (n_left > 0)
  {
    n_done=write(afd,ptr,n_left);
    if (n_done < 0)
    {
      perror("Write Error ");
      exit(0);
    }
    n_left-=n_done;
    ptr+=n_done;
  }
}

int TCP_read(int afd, char *msg)
{
  int n_left, n_done;
  char *ptr;
  ptr=msg;
  n_left=BUFFERSIZE;
  while (n_left > 0)
  {
    n_done=read(afd,ptr,n_left);
    if (n_done < 0)
    {
      perror("Read Error ");
      return SERV_TROUBLE;
    }
    else if (n_done == 0)
    {
      if(n_left==BUFFERSIZE)
        return SERV_TROUBLE;
      break;
    }
    n_left-=n_done;
    //If the TCP protocolled messages' terminator (newline) is found, stop the reading process
    if (strchr(ptr,'\n') != NULL)
      break;
    ptr+=n_done;
  }
  return SERV_OK;
}

int UDP_contact(char *msg, struct sockaddr_in serveraddr, int afd, char *buffer)
{
  socklen_t addrlen;
  int sent_bytes, recv_bytes, try = 3, counter;
  struct timeval cntdwn;
  fd_set irfds;
  //Create a tolerance of 3 attempts to get a response to the sent message
  while (try)
  {
    //Each will have a 3 seconds window to obtain a response
    cntdwn.tv_sec=3;
    cntdwn.tv_usec=0;
    sent_bytes=sendto(afd, msg, strlen(msg), 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (sent_bytes == -1)
    {
      perror("Error ");
      return SERV_TROUBLE;
    }
    FD_ZERO(&irfds);
    FD_SET(afd,&irfds);
    counter=select(afd+1,&irfds,(fd_set*)NULL,(fd_set*)NULL,&cntdwn);
    if (counter < 0)
    {
      perror("Select Error ");
      return SERV_TROUBLE;
    }
    else if (!counter)
    {
      printf("Send UDP Error: Sending again\n");
      try--;
    }
    else
      break;
  }
  if (FD_ISSET(afd,&irfds))
  {
    addrlen = sizeof(serveraddr);
    recv_bytes=recvfrom(afd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&serveraddr, &addrlen);
    if (recv_bytes == -1)
    {
      perror("Error ");
      return SERV_TROUBLE;
    }
    buffer[recv_bytes]='\0';
    return SERV_OK;
  }
  else
  {
    printf("SELECT TROUBLE\n");
    return SERV_TROUBLE;
  }
}
