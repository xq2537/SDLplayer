
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "misc.h"

char *concat_str_short(char *str, short num)
{
  char strnum[12];
  char *value;

  memset(strnum, 0, sizeof(*strnum) * 12);
  sprintf(strnum, "%i", num);
  if (!(value = malloc((strlen(str) + strlen(strnum)) * sizeof(*value))))
    {
      return (0);
    }
  strcpy(value, str);
  strcat(value, strnum);
  return (value);
}
