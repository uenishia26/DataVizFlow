#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <stdbool.h>
#include <string.h> 
#define MAX_DATA_LENGTH 20 //Length of the Data name=Value / max length of name / max length of Value
#define MAX_PAIRS 50 //Number of nameValuePairs 
#define MAX_PAIRS_PER_SAMPLE 50 //The maximum number of sample Data
#define MAX_NUM_OF_SAMPLES 50 
#define MAX_UNIQUE_NAMES 50


//Struct that stores parsed data
typedef struct
{
    char name[MAX_DATA_LENGTH];
    char value[MAX_DATA_LENGTH];
}NameValuePair;

//SampleData (contains a list of NameValuePairs) For each sample can hold up to 50 NameValPair
typedef struct 
{
    NameValuePair sample[MAX_PAIRS_PER_SAMPLE]; 
    int numOfNameValuePair; 
}sampleData; 





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

NameValuePair *readFromBuffer(ringBuffer *rb)
{    
    
    sem_wait(&rb->filledSlots); 
    NameValuePair *source = (NameValuePair *)(rb->buffer + rb->head*sizeof(NameValuePair)); 
    rb->head = ((rb->head+1) % rb->bufferSize);  
    sem_post(&rb->emptySlots);   
    return source; 
}




int main(int argc, char *argv[])
{
    printf("In process 2\n"); 
    //Receiving shmid for shared buffer + Creating ringBuffer
    int shmid = atoi(argv[1]);     
    void *test = shmat(shmid, NULL, 0);  
    if(test == (void *)-1) 
       printf("There is an error with shmid"); 
    ringBuffer * rb = (ringBuffer *)test;
    int currentSampleIndex = 0; 
    bool endNameIdentified = false; 
    char endName [20];
    int currentSampleDataIndex = 0;  
    int prevSampleNVPIndex = 0; //Keeps track of index when incremneting through the prevSample 

    //Creating an Array of sampleData strucutures, each sampleData strucutre can hold up to 50 NAMEVALPAIRS 
    sampleData sd [MAX_NUM_OF_SAMPLES]; 

   
    //Intializing sampleData  
    for(int x = 0; x < MAX_NUM_OF_SAMPLES; x++)
    {
        
        for(int i = 0; i < MAX_PAIRS_PER_SAMPLE; i++)
        {
            sd[x].sample[i].name[0]= '\0'; 
            sd[x].sample[i].value[0] = '\0'; 
            sd[x].numOfNameValuePair = 0; 
        }

    }
    
    for(int x = 0; x < 6; x++){    
        NameValuePair *tempNVP; 
        tempNVP = readFromBuffer(rb); 

        //End name has not been identified yet 
        if(endNameIdentified == false)
        {
            //Checking if name EXIST in DATASAMPLE to determine endName 
            for(int sampleIndex = 0; sampleIndex < sd[0].numOfNameValuePair; sampleIndex++)
            {
                if(strcmp(sd[0].sample[sampleIndex].name, tempNVP->name)== 0)
                {
                    endNameIdentified = true; 
                    strncpy(endName, tempNVP->name, MAX_DATA_LENGTH); 
                    break; 
                }

            }
            if(endNameIdentified == false)
            {
                strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, tempNVP->name, MAX_DATA_LENGTH); 
                strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, tempNVP->value, MAX_DATA_LENGTH); 
                sd[currentSampleDataIndex].numOfNameValuePair = sd[currentSampleDataIndex].numOfNameValuePair + 1; 
            }
            else 
            {   
                currentSampleDataIndex++; 
            }
                
        }
        //EndName has been identified || the first time we identified sampeIndex we need to not forget NameValuePair 
        else
        {
            int prevSample = currentSampleDataIndex - 1; 

            //Use prevSample DATA for CurrentSampleData 
            while(strcmp(sd[prevSample].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, tempNVP->name) != 0)
            {
                strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, 
                    sd[prevSample].sample[sd[currentSampleDataIndex].numOfNameValuePair].name,
                    MAX_DATA_LENGTH); 

                strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, 
                    sd[prevSample].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, 
                    MAX_DATA_LENGTH); 

                sd[currentSampleDataIndex].numOfNameValuePair = sd[currentSampleDataIndex].numOfNameValuePair + 1; 
                
            }

            //Adding new Data to the currentSample 
            strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, tempNVP->name, MAX_DATA_LENGTH); 
            strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, tempNVP->value, MAX_DATA_LENGTH); 
            sd[currentSampleDataIndex].numOfNameValuePair = sd[currentSampleDataIndex].numOfNameValuePair + 1; 

            //If we have reached the end name, we start again with a new Sample set {}
            if(strcmp(tempNVP->name, endName) == 0)
            {
                currentSampleDataIndex++; 
            }
        }

    }
    


    for(int x = 0; x < 4; x++)
    {
        for(int y = 0; y < 4; y++)
        {
            printf("name: %s", sd[x].sample[y].name);  
            printf(" value: %s\n", sd[x].sample[y].value); 
            sleep(1);
        }
        
    }  

    printf("%s", endName); 
    
    


    return 0; 
}