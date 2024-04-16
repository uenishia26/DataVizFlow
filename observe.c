#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <stdbool.h>
#define MAX_DATA_LENGTH 20 //Length of the Data name=Value / max length of name / max length of Value
#define MAX_PAIRS 10000 //Number of nameValuePairs 
#define MAX_SLOT_LENGTH 1000//Max lenght of each slot of the buffer 




//Struct that stores parsed data
typedef struct
{
    char name[MAX_DATA_LENGTH];
    char value[MAX_DATA_LENGTH];
}NameValuePair;

void addNameValuePair(int index, NameValuePair *pairs, char *name, char *value)
{
    strncpy(pairs[index].name, name, MAX_DATA_LENGTH);
    pairs[index].name[strlen(name)] = '\0';
    strncpy(pairs[index].value, value, MAX_DATA_LENGTH);
    pairs[index].name[strlen(name)] = '\0';
}

typedef enum {bit0=0, bit1=1} bit;

//Struct for ringBuffer / necessary ringBuffer functions
typedef struct
{
  bit latest;
  bit reading;
  bit slot[2];
  sem_t filledSlots;
  sem_t emptySlots;
  int head;
  int tail;
  int bufferSize;
  char buffer[];
}ringBuffer;

//Write to Process 2
void writeToBuffer(ringBuffer *rb, char *str)
{ 
    sem_wait(&rb->emptySlots); 
    strncpy(rb->buffer + rb->tail*MAX_SLOT_LENGTH, str, MAX_SLOT_LENGTH);  
    rb->tail = (rb->tail+1) % rb->bufferSize; 
    sem_post(&rb->filledSlots); 
    //printf("Writting In Process 1: %s \n", str);
    //sleep(1); 
}

void bufwrite(ringBuffer *sb, char *item)
{
  bit pair, index;
  pair = !sb->reading;
  index = !sb->slot[pair];

  //printf("Writing Process 1: %s\n", item);
  strncpy(sb->buffer + 2*pair*MAX_SLOT_LENGTH + index*MAX_SLOT_LENGTH, item, MAX_SLOT_LENGTH);
  //printf("Index: %d, Value: %s\n", 2*pair*MAX_SLOT_LENGTH + index*MAX_SLOT_LENGTH, sb->buffer + 2*pair*MAX_SLOT_LENGTH + index*MAX_SLOT_LENGTH);
  sb->slot[pair] = index;
  sb->latest = pair;
  usleep(100000); 
}


int main(int argc, char *argv[])
{
  //printf("In Process 1\n");
    //Receiving shmid for shared buffer + Creating ringBuffer
  int shmid = atoi(argv[1]);
  char *sync = argv[2];
    void *test = shmat(shmid, NULL, 0);  
    if(test == (void *)-1) 
       printf("There is an error with shmid");
    ringBuffer * sb;
    ringBuffer * rb;

    if (strcmp(sync, "async") == 0)
      {
	 sb = (ringBuffer *)test;
      }
    else
      {
	rb = (ringBuffer *)test;
      }



    //Strtok: stdin Parsing (Similar to a substring function) + Setup for parsing 
    char line[MAX_DATA_LENGTH];
    NameValuePair pairs [MAX_PAIRS];
    int index = 0;
    int numOfUniqueNames = 0;
    bool nameExist = false;
    bool setDetermined = false;
    int x;

    //FILE *file = fopen("testFile", "r"); 
    

    while(fgets(line, sizeof(line), stdin))
    {
        char *name = strtok(line, "=");
        char *value = strtok(NULL, "\n"); //Null = contine searching the same string
        nameExist = false;
        for(x = 0; x < index; x++)
        {
            /* If a name already exists in the set, this means set is now determined
            thus no more unique names can be added
            */
            if(strcmp(pairs[x].name, name) == 0)
            {
               nameExist = true;
               setDetermined = true;
               break;  
            }
        }
        if(nameExist)
        {
            if(strcmp(pairs[x].value, value) != 0)
            {
                addNameValuePair(index, pairs, name, value);
                index++;
            }
        }
        //The name must be unique && the set size cannot be determined
        else if(setDetermined == false)
        {
           
            addNameValuePair(index, pairs, name, value);
            numOfUniqueNames++;
            index++;
        }  
    }




    //Now we can begin to write into the shared buffer but first combine the name value pairs with = back to its original format 
    for(int x = 0; x < index; x++)
    {
        //We remake the string 
        char nameValCombined [MAX_DATA_LENGTH*2]; //Cause its name and vale thus technically max can be 2 times 
        sprintf(nameValCombined, "%s=%s", pairs[x].name, pairs[x].value);
	if (strcmp(sync, "async") == 0)
	  {
	    bufwrite(sb, nameValCombined);
	  }
	else
	  {
	    writeToBuffer(rb, nameValCombined);
	  }
    }

    //This is to indicate termination 
    if (strcmp(sync, "async") == 0)
      {
	bufwrite(sb, "EOF");
      }
    else
      {
	writeToBuffer(rb, "EOF");
      }
   
    return 0;
}
