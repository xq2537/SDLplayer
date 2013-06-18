
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "sensor_listener.h"
#include "misc.h"
#include "adb.h"
#include "socket.h"

#ifndef WIN32
#include <pthread.h>
#endif

void our_sleep(int secs) {
#ifdef WIN32
    Sleep(secs);
#else
    sleep(secs);
#endif
}

static void listen_sensor_app(short port)
{
  SOCKET guest_sock;
  SOCKET sock;
  char *data;

  printf("opening server on port 22470\n");
  while ((guest_sock = open_server("22470")) == INVALID_SOCKET) 
    {
      printf("unable to listen on port 22470... retyring in few seconds\n");
      our_sleep(5);
    }

  while (1)
    {
      printf("Waiting for connection from VM\n");
      SOCKET client_sock = connect_client(guest_sock);
      if (client_sock == INVALID_SOCKET)
	{
	  perror("accept()");
	  continue;
	}
      printf("Got sensors connection from VM\n");
      while (1) 
	{
	  printf("Connecting to hardware device on port %d...\n", port);
	  sock = open_socket("127.0.0.1", port);
	  if (sock != INVALID_SOCKET)
	    {
	      printf("Connected to hardware device, socket=%d :-)\n", sock);
	      while ((data = read_socket(sock, PACKET_LEN)))
		{
		  if (write_socket(client_sock, data, PACKET_LEN)<0)
		    {
		      printf("Connection closed with VM - closing connection with hardware device\n");
		      free(data);
	            }
	        }
	      printf("Closing connection to hardware device\n");
	      closesocket(sock);
	      our_sleep(2);
	    }
	  else
	    {
	      printf("Unable to connect to hardware device\n");
	      closesocket(client_sock);
	      closesocket(guest_sock);
	      return;
	    }
	    
	}
    }
  closesocket(guest_sock);
}

static void *start_sensor_listener_method(void *args)
{
  sensor_listener_params *data;
  short port;
  int try;
  int ret;
  char *default_params[] = {
    "adb",
    "-s",
    "forward",
    "tcp:" ANDROIDAPP_PORT,
  };
  char **params;

  try = 0;
  port = 28998;
  data = args;

  while (1)
    {
      char *strport;

      if (!(strport = concat_str_short("tcp:", port)))
	{
	  fprintf(stderr, "Not enough memory\n");
	  exit(1);
	}
      if (!(params = malloc(7 * sizeof(*params))))
	{
	  free(strport);
	  fprintf(stderr, "Not enough memory\n");
	  exit(1);
	}
      params[0] = default_params[0];
      params[1] = default_params[1];
      params[2] = data->device;
      params[3] = default_params[2];
      params[4] = strport;
      params[5] = default_params[3];
      params[6] = 0;

      ret = run_command(params);
      printf("forwarding on port %i\n", port);
      free(params);
      free(strport);
      if (!ret)
	{
          printf("ADB connection configured\n");
	  listen_sensor_app(port);
          continue;
	}
      printf("Error configuring ADB connection - is device USB connected with debug enabled ?\n");
      our_sleep(10);
      port += ++try;
    }
  free(data);
  return (0);
}

void *start_sensor_listener(char *device_name)
{
#ifdef WIN32
  return (0);
#else
  pthread_t *thread;
  sensor_listener_params *params;

  if (!(thread = malloc(sizeof(*thread))) || !(params = malloc(sizeof(*params))))
    {
      fprintf(stderr, "Out of memory\n");
      exit(1);
    }
  params->device = device_name;
  if (!pthread_create(thread, 0, &start_sensor_listener_method, params))
    {
      return (thread);
    }
  else
    {
      return (0);
    }
#endif
}
