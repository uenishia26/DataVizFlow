#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <stdbool.h>
#define MAX_DATA_LENGTH 20 //Length of the Data name=Value / max length of name / max length of Value
#define MAX_PAIRS 50 //Number of nameValuePairs 



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

//Struct for ringBuffer / necessary ringBuffer functions
typedef struct 
{
  sem_t filledSlots;
  sem_t emptySlots;    
  int head; 
  int tail; 
  int bufferSize; 
  char buffer[];  
}ringBuffer; 

void writeToBuffer(ringBuffer *rb, NameValuePair *nvp)
{ 
    sem_wait(&rb->emptySlots); 
    memcpy(rb->buffer + rb->tail*sizeof(NameValuePair), nvp, sizeof(NameValuePair));
    rb->tail = (rb->tail+1) % rb->bufferSize; 
    sem_post(&rb->filledSlots); 
    printf("Writting: %s %s\n", nvp->name, nvp->value);
    sleep(1); 
}






int main(int argc, char *argv[])
{   
    //Receiving shmid for shared buffer + Creating ringBuffer
    int shmid = atoi(argv[1]);     
    void *test = shmat(shmid, NULL, 0);  
    if(test == (void *)-1) 
       printf("There is an error with shmid"); 
    ringBuffer * rb = (ringBuffer *)test;




    //Strtok: stdin Parsing (Similar to a substring function) + Setup for parsing 
    char line[MAX_DATA_LENGTH];
    NameValuePair pairs [MAX_PAIRS];
    int index = 0;
    int numOfUniqueNames = 0;
    bool nameExist = false;
    bool setDetermined = false;
    int x;

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
        
        //char nameValCombined [MAX_DATA_LENGTH*2]; //Cause its name and vale thus technically max can be 2 times 
        //sprintf(nameValCombined, "%s=%s", pairs[x].name, pairs[x].value); 


        writeToBuffer(rb, &pairs[x]); 
    }
   
    return 0;
}