#include "libobjdata.h"

void init_buffer (buffer_t *b)
{
  b->in_marker = b->out_marker = b->count = 0;

  for (int i = 0; i < b->buff_size; i++)
  {
    b->slot[i].data = (char *) malloc (24);
    b->slot[i].size = 0;
  }

  pthread_mutex_init (&b->mutex, NULL);
  pthread_cond_init (&b->occupied_slot, NULL);
  pthread_cond_init (&b->empty_slot, NULL);
}

int main(int argc, char *argv[])
{
  int options;
  static pthread_attr_t tattr; // thread attributes. USed for controling scheduling
  static struct sched_param tp;
  void *lib_handle;
  char *funcList [1000];
  int funcIndex = 0;
  int size = 1;
  char *typeBuffer;
  lib_handle = dlopen("./libobjdata.so", RTLD_NOW);
  
  if (!lib_handle)
  {
    dlerror();
    return 1;
  }

  for(int index = 1; index < argc; index++)
  {
    if(strncmp(argv[index], "-t", 2) == 0)
    {
      funcList[funcIndex] = argv[++index]; 
      funcIndex++; 
    }
    else
    {   //Set getopt to start from this index 
      optind = index; 
      break; 
    }
  }

  while((options=getopt(argc, argv, "b:s:"))!=-1) 
  {
    switch(options)
    {
      case 'b':
	typeBuffer = optarg; 
	break; 
      
      case 's':
	size = atoi(optarg); 
	break; 
      
      case '?':
	printf("Unknown option"); 
	break; 
    }
  } 

  thread_table = (pthread_t*)malloc((funcIndex) * sizeof(pthread_t));
  if (thread_table == NULL)
  {
    fprintf (stderr, "Cannot allocate memory for thread_table");
    exit (1);
  }
  //initial the buffer
  buff = (buffer_t*)malloc(sizeof(buffer_t) + sizeof(slot_t) * size);
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

  for (int i = 0; i < funcIndex; i++)
  {
    struct local_file* (*func)(void);
    const char* error_msg;
    func = dlsym(lib_handle, funcList[i]);
    error_msg = dlerror();
    if (error_msg)
    {
      dlerror();
      dlclose(lib_handle);
      return 1;
    }

    if (pthread_create (&thread_table[i], &tattr, (void *(*)(void *))func, buff) != 0)
    {
      perror ("Unable to create thread");
      exit (1);
    }
  }

  for (int i = 0; i < funcIndex; i++)
  {
    pthread_join (thread_table[i], NULL);
  }

  // We don't need our mutex or condition variables any more
  pthread_mutex_destroy (&buff->mutex);
  pthread_cond_destroy (&buff->occupied_slot);
  pthread_cond_destroy (&buff->empty_slot);
  free(thread_table);
  free(buff);
  dlclose(lib_handle);

  return 0;
}
