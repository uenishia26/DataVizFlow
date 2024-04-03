#include "libobjdata.h"

void *put_data (void *arg)
{
  buff = (buffer_t *) arg;
  int i = 0;
  char s[80];

  while (i < 2)
  {
    sprintf (s, "Current count is %d, producer is %lu\n", i, pthread_self());
    produce (buff, s, strlen(s)+1);
    i++;
  }
}
