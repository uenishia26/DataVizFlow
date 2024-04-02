#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

typedef struct
{
  char *data; //buffer slot stored data
  size_t size; //size of the data
  pthread_t id; //destination thread id
} slot_t;

typedef struct
{
  int buff_size; //size of buffer
  int count; //number of slots in the buffer
  int in_marker; //next slot to add data to
  int out_marker; //next slot to take data from
  pthread_mutex_t mutex; //mutex for shared buffer
  pthread_cond_t occupied_slot; //condition when >= 1 full buffer slots
  pthread_cond_t empty_slot; //condition when at least 1 empty buffer slot
  slot_t slot[]; //array of slots
} buffer_t;

typedef struct
{
  pthread_t producer; //creating producer identifier	    
  pthread_t consumer; //creating consumer identifier
} thr_name_t;

buffer_t *buff;

int producers = 1;
int total_msgs = 1;
thr_name_t *thread_table;

pthread_t lookup (pthread_t id);

void produce (buffer_t *b, char *item, size_t size, pthread_t target)
{  
  pthread_mutex_lock (&b->mutex);

  while (b->count >= b->buff_size)
  {
    pthread_cond_wait (&b->empty_slot, &b->mutex);
  }

  assert (b->count < b->buff_size);

  b->slot[b->in_marker].data = (char *) malloc (size);
  b->slot[b->in_marker].size = size;
  b->slot[b->in_marker].id   = target;

  //printf("%s: item is:%s", __FUNCTION__, item);

  memcpy (b->slot[b->in_marker].data, item, size);
  b->in_marker++;
  b->in_marker %= b->buff_size;
  b->count++;

  pthread_cond_broadcast (&b->occupied_slot);
  
  pthread_mutex_unlock (&b->mutex);
  
}

char *consume (buffer_t *b)
{
  char *item;
  pthread_t self=pthread_self();

  pthread_mutex_lock (&b->mutex);
  while (b->count <= 0)
    pthread_cond_wait (&b->occupied_slot, &b->mutex);
  
  assert (b->count > 0);

  /* Check id of target thread to see message is for this thread.        */    
  if (!pthread_equal(b->slot[b->out_marker].id, self))
  {
    /* Data is not for this thread. */

    //printf ("%s: slot id: %lu, consumer id: %lu\n",
    //	    __FUNCTION__,
    //	    b->slot[b->out_marker].id,
    //	    self);
    pthread_mutex_unlock (&b->mutex);
    
    return NULL;
  }
  
  item = (char *) malloc (b->slot[b->out_marker].size);

  memcpy (item, b->slot[b->out_marker].data, b->slot[b->out_marker].size);
  free (b->slot[b->out_marker].data);
  b->out_marker++;
  b->out_marker %= b->buff_size;
  b->count--;

  pthread_cond_broadcast (&b->empty_slot);

  pthread_mutex_unlock (&b->mutex);

  return (item);
}

pthread_t lookup (pthread_t id)
{  
  for (int i = 0; i < producers; i++)
  {
    if (pthread_equal (thread_table[i].producer, id))
    {
      return (thread_table[i].consumer);
    }
  }
  
  return (-1);
}

void get_data (void)
{
  char *data;
  int i = 0;
  
  while (i < total_msgs)
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

void put_data ()
{  
  int i = 0;
  char s[80];
  pthread_t self=pthread_self();
  pthread_t target;

  target = lookup (self);

  while (i < total_msgs)
  {
    sprintf (s, "Current count is %d, producer is %lu, target is %lu\n", i, self, target);
    produce (buff, s, strlen(s)+1, target);
    i++;
  }
}

void init_buffer (buffer_t *b)
{
  b->in_marker = b->out_marker = b->count = 0;

  for (int i = 0; i < b->buff_size; i++)
  {
    b->slot[i].data = NULL;
  }

  pthread_mutex_init (&b->mutex, NULL);
  pthread_cond_init (&b->occupied_slot, NULL);
  pthread_cond_init (&b->empty_slot, NULL);
}

int main(int argc, char *argv[])
{

  static pthread_attr_t tattr; // thread attributes. USed for controling scheduling
  static struct sched_param tp;
  
  //get the value of the len and the number of threads
  int size = 0;
  if (!(argc == 3 || argc == 4)) //check if they specify the size of buff, number of producer and msges to consumer
  {
    fprintf (stderr, "Usage: %s [number_of_producers msgs_per_consumer number_of_slot]\n", argv[0]);
    exit (1);
  }
  else
  {
    if (argc == 4)
    {
      size = atoi(argv[argc - 1]);
    }
    else
    {
      size = 1;
    }
    producers = atoi (argv[1]);
    total_msgs = atoi (argv[2]);
  }

  thread_table = (thr_name_t*)malloc((producers) * sizeof(thr_name_t));
  if (thread_table == NULL)
  {
    fprintf (stderr, "Cannot allocate memory for thread_table");
    exit (1);
  }
  //initial the buffer
  buff = (buffer_t*)malloc(sizeof *buff + sizeof buff->slot[0] * (size));
  if (buff == NULL)
  {
    fprintf (stderr, "Cannot allocate memory for buff");
    exit(1);
  }
  buff->buff_size = size;
  init_buffer(buff);

  //prepare thread attributes
  if (pthread_attr_setschedpolicy(&tattr, SCHED_RR)) //Set the scheduling policy to red robin
  {
    fprintf (stderr, "Cannot set thread scheduling policy!\n");
    exit (1);
  }

  tp.sched_priority = 50;
  if (pthread_attr_setschedparam(&tattr,  &tp)) //set priority between 0 and 99
  {
    fprintf (stderr, "Cannot set thread scheduling priority!\n");
    exit (1);
  }

  if (pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM)) //Now set the thread scope to be system-wide
  {
    fprintf (stderr, "Cannot set thread execution scope!\n");
    exit (1);
  }

  for (int i = 0; i < producers; i++)
  {
    if (pthread_create (&thread_table[i].consumer, &tattr,(void *(*)(void *))get_data, NULL) != 0)
    {
      perror ("Unable to create thread");
      exit (1);
    }
  }

  for (int i = 0; i < producers; i++)
  {
    if (pthread_create (&thread_table[i].producer, &tattr, (void *(*)(void *))put_data, NULL) != 0)
    {
      perror ("Unable to create thread");
      exit (1);
    }
  }

  for (int i = 0; i < producers; i++)
  {
    pthread_join (thread_table[i].producer, NULL);
    pthread_join (thread_table[i].consumer, NULL);
  }

  // We don't need our mutex or condition variables any more
  pthread_mutex_destroy (&buff->mutex);
  pthread_cond_destroy (&buff->occupied_slot);
  pthread_cond_destroy (&buff->empty_slot);
  free(thread_table);
  free(buff);

  return 0;
}
