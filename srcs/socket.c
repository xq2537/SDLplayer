
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <errno.h>

#include "socket.h"

//#include "vmci_sockets.h"

SOCKET open_socket(char *ip, short port)
{
  SOCKET sock;
  SOCKADDR_IN sin;
  struct hostent *hostinfo;
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
      return (INVALID_SOCKET);
    }

  if (!(hostinfo = gethostbyname(ip)))
    {
      closesocket(sock);
      return (INVALID_SOCKET);
    }

  memset(&sin, 0, sizeof(sin));
  sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == INVALID_SOCKET)
    {
      closesocket(sock);
      return (INVALID_SOCKET);
    }

  return (sock);
}

SOCKET open_socket_from_sockaddr(SOCKADDR_IN sin, short port)
{
  SOCKET sock;
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
      return (INVALID_SOCKET);
    }

  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == INVALID_SOCKET)
    {
      closesocket(sock);
      return (INVALID_SOCKET);
    }

  return (sock);
}

SOCKET open_server(char *port)
{
  SOCKET server_sock;
  int option_value;
  struct sockaddr_in sin;
  
  server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock == INVALID_SOCKET)
    {
      perror("socket() server");
      exit(1);
    }

  option_value = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) < 0) 
    {
      fprintf(stderr, "Unable to setsockopt SO_REUSEADDR, errno=%d\n", errno);
    }

  memset(&sin, 0, sizeof(sin));
  sin.sin_port = htons(atoi(port));
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_family = AF_INET;

  if (bind(server_sock, &sin, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
      perror("bind()");
      return INVALID_SOCKET;
    }

  if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR)
    {
      perror("listen()");
      return INVALID_SOCKET;
    }
  return (server_sock);
}

#if 0
SOCKET open_vmware_server(char *port)
{
  SOCKET server_sock;
  int afVMCI = VMCISock_GetAFValue();
  struct sockaddr_vm vm_addr;
  int option_value;

  memset(&vm_addr, 0, sizeof(vm_addr));
  vm_addr.svm_family = afVMCI;
  sscanf(port, "%d", &(vm_addr.svm_port));
  vm_addr.svm_port = htons(vm_addr.svm_port);
  vm_addr.svm_cid = VMCISock_GetLocalCID();
  
  server_sock = socket(afVMCI, SOCK_STREAM, 0);
  if (server_sock == INVALID_SOCKET)
    {
      perror("socket() server");
      exit(1);
    }

  option_value = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value));
  if (bind(server_sock, (struct sockaddr *)&vm_addr, sizeof(vm_addr)) == SOCKET_ERROR)
    {
      perror("bind()");
      exit(1);
    }

  if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR)
    {
      perror("listen()");
      exit(1);
    }
  return (server_sock);
}
#endif 

SOCKET connect_client(SOCKET server)
{
  SOCKADDR_IN client_addr;
  socklen_t client_addr_len;

  client_addr_len = sizeof(client_addr);

  return (accept(server, (SOCKADDR *)&client_addr, &client_addr_len));
}

static void wait_for_data(SOCKET sock)
{
  fd_set forread;

  while (1)
    {
      FD_ZERO(&forread);
      FD_SET(sock, &forread);
      if (select(sock + 1, &forread, 0, 0, 0) == -1)
	{
	  perror("select()");
	  exit(1);
	}
      if (FD_ISSET(sock, &forread))
	{
	  return;
	}
    }
}

char *read_socket(SOCKET sock, int maxlen)
{
  char *buffer;
  int len = 0;

  if (!(buffer = malloc((maxlen + 1) * sizeof(*buffer))))
    {
      return (0);
    }
  wait_for_data(sock);
  if ((len = recv(sock, buffer, maxlen * sizeof(*buffer), MSG_WAITALL)) <= 0)
   {
     fprintf(stderr, "Error in recv() - len=%d errno=%d\n", len, errno);
     free(buffer);
     return (0);
   }
  buffer[len] = 0;
  return (buffer);
}

int write_socket(SOCKET sock, char *data, int len)
{
  return (send(sock, data, len, 0));
}
