#ifndef UNTITLED_LIBOBJDATA_H
#define UNTITLED_LIBOBJDATA_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <dlfcn.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <bfd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

typedef struct
{
  char *data; //buffer slot stored data
  size_t size; //size of the data
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

#endif

buffer_t *buff;
pthread_t *thread_table;

void *put_data (void *arg);

void *get_data (void *arg);

void produce (buffer_t *b, char *item, size_t size);

char *consume (buffer_t *b);
