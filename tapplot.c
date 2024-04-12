#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#define MAX_DATA_LENGTH 20 //Length of the Data name=Value / max length of name / max length of Value
#define MAX_PAIRS 20 //Number of nameValuePairs 
#define MAX_PAIRS_PER_SAMPLE 10 //The maximum number of sample Data
#define MAX_NUM_OF_SAMPLES 20 
#define MAX_UNIQUE_NAMES 50
#define MAX_SLOT_LENGTH 1000//Max lenght of each slot of the buffer


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

typedef enum {bit0=0, bit1=1} bit;


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


//Read from Process 2
char *readFromBuffer(ringBuffer *rb)
{    
    sem_wait(&rb->filledSlots); 
    char *source = (char *)(rb->buffer + rb->head*MAX_SLOT_LENGTH); 
    printf("Reading: %s\n", source); 
    rb->head = ((rb->head+1) % rb->bufferSize);  
    sem_post(&rb->emptySlots);  
    return source; 
}

char *bufread(ringBuffer *sb)
{
  bit pair, index;
  pair = sb->latest;
  sb->reading = pair;
  index = sb->slot[pair];
  char *item = (sb->buffer + 2*pair*MAX_SLOT_LENGTH + index*MAX_SLOT_LENGTH);
  printf("Reading in PROCESS 3: %s\n", item);
  printf("Index: %d, Value: %s\n", 2*pair*MAX_SLOT_LENGTH + index*MAX_SLOT_LENGTH, item);
  sleep(1);
  return (item);
}

NameValuePair parseSampleDataStr(char *sampleData, int argn)
{
    NameValuePair eofCheck; 
    if(strcmp(sampleData, "EOF")==0)
    {
        strncpy(eofCheck.name, "EOF", MAX_DATA_LENGTH); 
        return eofCheck; 
    }

    if (strcmp(sampleData, "")==0)
      {
	strncpy(eofCheck.name, "", MAX_DATA_LENGTH);
        return eofCheck;
      }

    int whichNameValuePair = 1; 
    char *subString; 

    while(subString = strtok_r(sampleData ,",", &sampleData))
    {
        //If its the nameValuePair we care about 
        //We create the nameValuePair and return it 
        if(whichNameValuePair == argn)
        {
            NameValuePair tempNVP; 
            char *name = strtok(subString, "="); 
            char *value = strtok(NULL, "="); 
            strncpy(tempNVP.name, name, MAX_DATA_LENGTH); 
            strncpy(tempNVP.value, value, MAX_DATA_LENGTH); 
            return tempNVP; 
        }
        else{
            whichNameValuePair++; 
        }
    }
}

int main(int argc, char *argv[])
{
    printf("In process 3\n"); 
    int p2P3shmid = atoi(argv[1]);   
    int argn = atoi(argv[2]);
    char *sync = argv[3];
    void *test = shmat(p2P3shmid, NULL, 0);  
    if(test == (void *)-1) 
       printf("There is an error with shmid"); 
    ringBuffer * sbP2P3;
    ringBuffer * rbP2P3;

    if (strcmp(sync, "async") == 0)
      {
         sbP2P3 = (ringBuffer *)test;
      }
    else
      {
        rbP2P3 = (ringBuffer *)test;
      }

    //Create/Open the file in append mode 
    FILE *file = fopen("dataFile.txt", "w"); 
    if(file == NULL)
    {
        perror("Error creating/opening the file"); 
        return 1; 
    }

    //Set up gnuplot by including one data input so gnuplot can determine a range 
    NameValuePair tempNVP;
    if (strcmp(sync, "async") == 0)
        {
          tempNVP = parseSampleDataStr(bufread(sbP2P3), argn);
        }
      else
        {
          tempNVP = parseSampleDataStr(readFromBuffer(rbP2P3), argn);
        }
    while (strcmp(tempNVP.name, "")==0)
    {
      if (strcmp(sync, "async") == 0)
	{
	  tempNVP = parseSampleDataStr(bufread(sbP2P3), argn);
	}
      else
	{
	  tempNVP = parseSampleDataStr(readFromBuffer(rbP2P3), argn);
	}
    }
    fprintf(file, "%d %s\n", 1, tempNVP.value); 
    fflush(file); 
    //system("gnuplot 'live_plot.gp' &"); //Allows for gnuplot to run in the background 

    int x = 1;  //This is sampleNumber 
    while(true)
    {
        NameValuePair tempNVP; 
        if (strcmp(sync, "async") == 0)
	  {
	      tempNVP = parseSampleDataStr(bufread(sbP2P3), argn);
	  }
	else
	  {
	      tempNVP = parseSampleDataStr(readFromBuffer(rbP2P3), argn);
	  }
	
	//Exit the loop as soon as we reach a EOF signal 
	if(strcmp(tempNVP.name, "EOF") == 0)
	  break; 
        
	fprintf(file, "%d %s\n", x+1, tempNVP.value); 
	fflush(file);   
	x++;
    }
 
    fclose(file); 
    return 0; 
}
