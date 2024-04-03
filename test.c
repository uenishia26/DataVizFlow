#include "libobjdata.h"

buffer_t* buff;

void *get_data (void *arg)
{
  buff = (buffer_t *) arg;
  char *data;
  int i = 0;
  
  while (i < 2)
  {
    printf ("My id is %lu: Message reads: %s", pthread_self(), data);
    free (data);
    i++;
  }
}

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

int main(int argc, char *argv[])
{
  return 1;
}
