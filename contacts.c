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
  printf("WROTE: %s (%d BYTES)\n", msg, n_done);
}

int TCP_read(int afd, char *msg)
{
  int n_left, n_done;
  char *ptr;
  ptr=msg;
  n_left=BUFFERSIZE;
  printf("GOT HERE\n");
  while (n_left > 0)
  {
    n_done=read(afd,ptr,n_left);
    printf("Reading %d bytes\n", n_done);
    if (n_done < 0)
    {
      perror("Read Error ");
      return SERV_TROUBLE;
    }
    else if (n_done == 0)
      break;
    n_left-=n_done;
    if (strchr(ptr,'\n') != NULL)
      break;
    ptr+=n_done;
  }
  printf("READ: %s (%d BYTES)\n", msg, BUFFERSIZE-n_left);
  return SERV_OK;
}

int UDP_contact(char *msg, struct sockaddr_in serveraddr, int afd, char *buffer)
{
  socklen_t addrlen;
  int sent_bytes, recv_bytes;
  struct timeval cntdwn;
  cntdwn.tv_sec=3;
  cntdwn.tv_usec=0;
  fd_set irfds;
  sent_bytes=sendto(afd, msg, strlen(msg), 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  if (sent_bytes == -1)
  {
    perror("Error ");
    return SERV_TROUBLE;
  }
  printf("SENT: %s (%d BYTES)\n",msg,sent_bytes);
  FD_ZERO(&irfds);
  FD_SET(afd,&irfds);
  select(afd+1,&irfds,(fd_set*)NULL,(fd_set*)NULL,&cntdwn);
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
    printf("RECEIVED: %s (%d BYTES)\n",buffer,recv_bytes);
    return SERV_OK;
  }
  else
  {
    printf("SELECT TROUBLE\n");
    return SERV_TROUBLE;
  }
}
