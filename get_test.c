#include "libobjdata.h"

void *get_data (void *arg)
{
  thread_arg *targ = (thread_arg *) arg;
  char *data;

  while ((data = consume (targ->buff[1])) == NULL)
  {
    sched_yield();
  }
  while ((data = consume (targ->buff[1])) != NULL)
  {
    printf ("Message reads: %s \n", data);\
    if (strcmp("EOF", data) == 0)
    {
      free (data);
      pthread_exit(NULL);
    }
    free (data);
  }
}
