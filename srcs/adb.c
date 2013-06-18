
#include <sys/types.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "adb.h"

int run_command(char **params)
{
#ifndef WIN32
  int status;
  pid_t pid;

  status = 0;
  pid = fork();

  if (pid == -1)
    {
      return (-1);
    }
  else if (pid == 0)
    {
      execvp("adb", params);
      exit(1);
    }
  else
    {
      wait(&status);
      return (WEXITSTATUS(status));
    }
	#endif
  return (0);
}
