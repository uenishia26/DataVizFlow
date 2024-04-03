#include "libobjdata.h"

void *get_data (void *arg)
{
  buff = (buffer_t *) arg;
  char *data;
  int i = 0;
  
  while (i < 2)
  {
     while ((data = consume (buff)) == NULL)
     {
      //printf ("%s: Waiting. Buffer count: %d, in: %d, out %d, slot id: %lu, tid: %lu\n",
      //	      __FUNCTION__, buffer.count, buffer.in_marker,
      //      buffer.out_marker,
      //      buffer.slot[buffer.out_marker].id,
      //      pthread_self());
    //sleep (1);
    //sched_yield();
    }
    printf ("My id is %lu: Message reads: %s", pthread_self(), data);
    free (data);
    i++;
  }
}
