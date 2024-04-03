#include "libobjdata.h"

buffer_t *buff;
pthread_t *thread_table;

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
  void *lib_handle;
  lib_handle = dlopen("./libobjdata.so", RTLD_NOW);
  
  if (!lib_handle)
  {
    dlerror();
    return 1;
  }
  
  //get the value of the len and the number of threads
  int size = 0;
  if (!(argc == 2)) //check if they specify the size of buff, number of producer and msges to consumer
  {
    size = 1;
  }
  else
  {
    size = atoi (argv[1]);
  }

  thread_table = (pthread_t*)malloc((2) * sizeof(pthread_t));
  if (thread_table == NULL)
  {
    fprintf (stderr, "Cannot allocate memory for thread_table");
    exit (1);
  }
  //initial the buffer
  buff = (buffer_t*)malloc(sizeof *buff + sizeof buff->slot[0] * (size + 1));
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
  
  struct local_file* (*put_data)(void);
  const char* error_msg;
  put_data = dlsym(lib_handle, "put_data");
  error_msg = dlerror();
  if (error_msg)
  {
    dlerror();
    dlclose(lib_handle);
    return 1;
  }
  struct local_file* (*get_data)(void);
  get_data = dlsym(lib_handle, "get_data");
  error_msg = dlerror();
  if (error_msg)
  {
    dlerror();
    dlclose(lib_handle);
    return 1;
  }

  if (pthread_create (&thread_table[0], &tattr, (void *(*)(void *))put_data, &buff) != 0)
  {
    perror ("Unable to create thread");
    exit (1);
  }

  if (pthread_create (&thread_table[1], &tattr, (void *(*)(void *))get_data, &buff) != 0)
  {
    perror ("Unable to create thread");
    exit (1);
  }

  printf("begin");

  pthread_join (thread_table[0], NULL);
  pthread_join (thread_table[1], NULL);

  printf("end");

  // We don't need our mutex or condition variables any more
  pthread_mutex_destroy (&buff->mutex);
  pthread_cond_destroy (&buff->occupied_slot);
  pthread_cond_destroy (&buff->empty_slot);
  free(thread_table);
  free(buff);

  return 0;
}
