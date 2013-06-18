
#ifndef __SENSOR_LISTENER_H_
# define __SENSOR_LISTENER_H_

# define ANDROIDAPP_PORT "23786"

# include "socket.h"

typedef struct s_sensor_listener_params {
  char *device;
  //  SOCKADDR_IN *guest;
} sensor_listener_params;

void *start_sensor_listener(char *device_name);//, SOCKADDR_IN *guest);
#endif
