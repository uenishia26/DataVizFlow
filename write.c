#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#define MAX_SLOT_LENGTH 32 

typedef struct 
{
  sem_t filledSlots;
  sem_t emptySlots;    
  int head; 
  int tail; 
  int bufferSize; 
  char buffer[];  
}ringBuffer; 

void writeToBuffer(ringBuffer *rb, char *valString)
{ 
    sem_wait(&rb->emptySlots); 
    strncpy(rb->buffer + rb->tail*MAX_SLOT_LENGTH, valString, MAX_SLOT_LENGTH);  
    rb->tail = (rb->tail+1) % rb->bufferSize; 
    sem_post(&rb->filledSlots); 
}
void readFromBuffer(ringBuffer *rb)
{
    sem_wait(&rb->filledSlots); 
    printf("%s", rb->buffer + rb->head*MAX_SLOT_LENGTH); 
    printf(" head: %d\n", rb->head); 
    rb->head = ((rb->head+1) % rb->bufferSize);  
    sem_post(&rb->emptySlots); 
}

int main(int argc, char *argv[])
{
    
    int shmid = atoi(argv[1]);     
    void *test = shmat(shmid, NULL, 0);  
    if(test == (void *)-1) 
       printf("There is an error with shmid"); 
    ringBuffer * rb = (ringBuffer *)test;
    printf("We are in the second process: \n"); 
    for(int x = 0; x < 10; x++) {
        readFromBuffer(rb); 
        fflush(stdout); 
        sleep(1); 
    } 
    


    

    return 0; 
}