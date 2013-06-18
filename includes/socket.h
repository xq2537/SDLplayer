
#ifndef __SOCKET_H_
# define __SOCKET_H_

# define PACKET_LEN 50

# ifdef WIN32

#  include <winsock2.h> 
#  include <Ws2tcpip.h>

#  ifndef MSG_WAITALL
#   define MSG_WAITALL 0
#  endif

# else

#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <sys/errno.h>
#  include <netdb.h>
#  include <unistd.h>

#  define closesocket(s) close(s)
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#  define h_addr h_addr_list[0]

typedef int			SOCKET;
typedef struct sockaddr_in	SOCKADDR_IN;
typedef struct sockaddr		SOCKADDR;
typedef struct in_addr		IN_ADDR;

# endif

typedef struct addrinfo		ADDRINFO;

SOCKET open_socket(char *ip, short port);
SOCKET open_socket_from_sockaddr(SOCKADDR_IN sin, short port);
SOCKET open_server(char *port);
char *read_socket(SOCKET sock, int len);
int write_socket(SOCKET sock, char *data, int len);
SOCKET connect_client(SOCKET server);

#endif
