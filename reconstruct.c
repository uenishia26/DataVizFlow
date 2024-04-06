#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <stdbool.h>
#include <string.h> 
#define MAX_SLOT_LENGTH 1000//Max lenght of each slot of the buffer 
#define MAX_DATA_LENGTH 15 //Length of the Data name=Value / max length of name / max length of Value
#define MAX_PAIRS 20 //Number of nameValuePairs 
#define MAX_PAIRS_PER_SAMPLE 10 //The maximum number of sample Data
#define MAX_NUM_OF_SAMPLES 20 
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


//NameValuePair structure 
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


//Read from Process 1 
char *readFromBuffer(ringBuffer *rb)
{    
    sem_wait(&rb->filledSlots); 
    char *source = (rb->buffer + rb->head*MAX_SLOT_LENGTH); 
    printf("Reading in PROCESS 2: %s\n", source); 
    rb->head = ((rb->head+1) % rb->bufferSize);  
    sem_post(&rb->emptySlots);   
    return source; 
}

//Write to Process 3 
void writeToBuffer(ringBuffer *rb, char *str)
{ 
    sem_wait(&rb->emptySlots);  
    printf("Writting to Process 3: %s\n", str); 
    strncpy(rb->buffer + rb->tail*MAX_SLOT_LENGTH, str, MAX_SLOT_LENGTH);  
    rb->tail = (rb->tail+1) % rb->bufferSize; 
    sem_post(&rb->filledSlots);
    sleep(1); 
}




int main(int argc, char *argv[])
{
    printf("In process 2\n"); 
    //Receiving shmid for shared buffer + Creating ringBuffer
    int shmid = atoi(argv[1]);    
    int p2p3Shmid = atoi(argv[2]); 

    //For memory between P1 & P2 ringBUffer called rb
    void *test = shmat(shmid, NULL, 0);  
    if(test == (void *)-1) 
       printf("There is an error with shmid"); 
    ringBuffer * rb = (ringBuffer *)test;

    //For memory between P2 & P3 ringBuffer called rbP2P3
    void *test2 = shmat(p2p3Shmid, NULL, 0);
    if(test == (void *)-1) 
       printf("There is an error with shmid"); 
    ringBuffer *rbP2P3 = (ringBuffer *)test2; 


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

   
    
    while(true)
    {
        char *str = readFromBuffer(rb); 
        //Checking if we reached an EOF signal
        if(strcmp(str, "EOF")==0)
        {
            //Checking if the sampleIndex is still 0 And We reached a EOF signal means one data input graph
            if(currentSampleDataIndex == 0)
                currentSampleDataIndex++; 
            break; 
        }

        //Reparsing the string again to there respective name and value pairs 
        char *name = strtok(str, "=");
        char *value = strtok(NULL, "="); //Null = contine searching the same string
        NameValuePair tempNVP; 
        strncpy(tempNVP.name, name, MAX_DATA_LENGTH); 
        strncpy(tempNVP.value, value, MAX_DATA_LENGTH);  

        //End name has not been identified yet 
        if(endNameIdentified == false)
        {
            //Checking if name EXIST in DATASAMPLE to determine endName 
            for(int sampleIndex = 0; sampleIndex < sd[0].numOfNameValuePair; sampleIndex++)
            {
                if(strcmp(sd[0].sample[sampleIndex].name, tempNVP.name)== 0)
                {
                    endNameIdentified = true; 
                    strncpy(endName, sd[0].sample[sd[0].numOfNameValuePair-1].name, MAX_DATA_LENGTH); 
                    break; 
                }

            }
            if(endNameIdentified == false)
            {
                strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, tempNVP.name, MAX_DATA_LENGTH); 
                strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, tempNVP.value, MAX_DATA_LENGTH); 
                sd[currentSampleDataIndex].numOfNameValuePair = sd[currentSampleDataIndex].numOfNameValuePair + 1; 
            }
            else 
            {   
                currentSampleDataIndex++; 
            }
                
        }
        //EndName has been identified or its the secondSampleData (Check ReadMe for explanation)
        if(currentSampleDataIndex == 1 || endNameIdentified == true)
        {
            int prevSample = currentSampleDataIndex - 1; 

            //Use prevSample DATA for CurrentSampleData 
            while(strcmp(sd[prevSample].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, tempNVP.name) != 0)
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
            strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, tempNVP.name, MAX_DATA_LENGTH); 
            strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, tempNVP.value, MAX_DATA_LENGTH); 
            sd[currentSampleDataIndex].numOfNameValuePair = sd[currentSampleDataIndex].numOfNameValuePair + 1; 

            //If we have reached the end name, we start again with a new Sample set {}
            if(strcmp(tempNVP.name, endName) == 0)
            {
                currentSampleDataIndex++; 
            }
        }

    } 


  
    //Creating a string that repersents each SampleData
    for(int sampleSet = 0; sampleSet < currentSampleDataIndex; sampleSet++)
    {
        //Creating a String that repersents each SampleData + Initalizing this data
        char sampleDataAsString[MAX_DATA_LENGTH*sd[0].numOfNameValuePair];
        sampleDataAsString[0] = '\0';  
        for(int sampleVal = 0; sampleVal < sd[sampleSet].numOfNameValuePair; sampleVal++)
        {
            //Stores a signle nameValPair (+1 for the null terminating sequence)
            char nameValCombined [MAX_DATA_LENGTH*2+1]; 
            //The conditions to ADD specifc commas between NameValuePairs
            if(sampleVal+1 == sd[sampleSet].numOfNameValuePair)
                sprintf(nameValCombined, "%s=%s", sd[sampleSet].sample[sampleVal].name, sd[sampleSet].sample[sampleVal].value); 
            else    
                sprintf(nameValCombined, "%s=%s,", sd[sampleSet].sample[sampleVal].name, sd[sampleSet].sample[sampleVal].value); 

            //Concatenate to the end of the sampleDataAsString structure 
            strncat(sampleDataAsString, nameValCombined, strlen(nameValCombined)); 
        }
        //Writing to the P2P3 Buffer Slot 
        printf("Hello"); 
        writeToBuffer(rbP2P3, sampleDataAsString); 
    }
    //EOF ending signal 
    writeToBuffer(rbP2P3, "EOF"); 


    


    return 0; 
}