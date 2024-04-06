#include "libobjdata.h"

/* Produce data referenced by `item' of `size' bytes and place in the
   buffer pointed to by `b'. The data is to be delivered to `target' 
   thread.                                                               */   
void produce (buffer_t *b, char *item, size_t size)
{
  
  pthread_mutex_lock (&b->mutex);

  if (b->count >= b->buff_size)
    pthread_cond_wait (&b->empty_slot, &b->mutex);

  assert (b->count < b->buff_size);

  b->slot[b->in_marker].data = (char *) malloc (size);
  b->slot[b->in_marker].size = size;

  //printf("%s: item is:%s", __FUNCTION__, item);

  printf("producing %s to buffer\n", item);
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
  if (b->count <= 0)
    pthread_cond_wait (&b->occupied_slot, &b->mutex);
  
  assert (b->count > 0);
  
  item = (char *) malloc (b->slot[b->out_marker].size);

  memcpy (item, b->slot[b->out_marker].data, b->slot[b->out_marker].size);
  printf("consuming %s from buffer\n", item);
  free (b->slot[b->out_marker].data);
  b->out_marker++;
  b->out_marker %= b->buff_size;
  b->count--;

  pthread_cond_broadcast (&b->empty_slot);

  pthread_mutex_unlock (&b->mutex);

  return (item);
}
