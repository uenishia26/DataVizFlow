#include "libobjdata.h"

void produce (buffer_t *b, char *item, size_t size)
{  
  pthread_mutex_lock (&b->mutex);

  while (b->count >= b->buff_size)
  {
    pthread_cond_wait (&b->empty_slot, &b->mutex);
  }

  assert (b->count < b->buff_size);

  b->slot[b->in_marker].data = (char *) malloc (size);
  b->slot[b->in_marker].size = size;

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

  pthread_mutex_lock (&b->mutex);
  while (b->count <= 0)
    pthread_cond_wait (&b->occupied_slot, &b->mutex);
  
  assert (b->count > 0);

  /* Check id of target thread to see message is for this thread.        */    
  pthread_mutex_unlock (&b->mutex);
  
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
