#include "contacts.h"

int UDP_contact(char *msg, struct sockaddr_in serveraddr, int afd, char *buffer)
{
  socklen_t addrlen;
  int sent_bytes, recv_bytes;
  struct timeval cntdwn;
  cntdwn.tv_sec=10;
  cntdwn.tv_usec=0;
  fd_set irfds;
  sent_bytes=sendto(afd, msg, strlen(msg), 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  if (sent_bytes == -1)
  {
    perror("Error: ");
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

int TCP_contact(char *msg, int afd, char *buffer)
{
  int sent_bytes, recv_bytes;
  struct timeval cntdwn;
  cntdwn.tv_sec=10;
  cntdwn.tv_usec=0;
  fd_set irfds;
  sent_bytes=write(afd, msg, strlen(msg));
  if (sent_bytes == -1)
  {
    perror("Error: ");
    return SERV_TROUBLE;
  }
  printf("SENT: %s (%d BYTES)\n",msg,sent_bytes);
  FD_ZERO(&irfds);
  FD_SET(afd,&irfds);
  select(afd+1,&irfds,(fd_set*)NULL,(fd_set*)NULL,&cntdwn);
  if (FD_ISSET(afd,&irfds))
  {
    recv_bytes=read(afd, buffer, BUFFERSIZE);
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
