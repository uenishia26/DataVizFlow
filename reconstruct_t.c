#include "libobjdata.h"
#define MAX_PAIRS 20 //Number of nameValuePairs 
#define MAX_PAIRS_PER_SAMPLE 10 //The maximum number of sample Data
#define MAX_NUM_OF_SAMPLES 20 
#define MAX_UNIQUE_NAMES 50

//SampleData (contains a list of NameValuePairs) For each sample can hold up to 50 NameValPair
typedef struct 
{
  NameValuePair sample[MAX_PAIRS_PER_SAMPLE]; 
  int numOfNameValuePair; 
}sampleData; 

void *reconstruct(void *arg)
{
  //printf("In process 2\n");   

  thread_arg *targ = (thread_arg *) arg; //get the arguments sent to thread
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
    char *str;
    if (targ->is_sync == 0)
    {
      str = consume (targ->buff[0]);
    }
    else if (targ->is_sync == 1)
    {
      str = slotread (targ->buff[0]);
    }

    if (strcmp(str, "") != 0)
    {
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
	  strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, sd[prevSample].sample[sd[currentSampleDataIndex].numOfNameValuePair].name, MAX_DATA_LENGTH); 
	
	  strncpy(sd[currentSampleDataIndex].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, sd[prevSample].sample[sd[currentSampleDataIndex].numOfNameValuePair].value, MAX_DATA_LENGTH); 
	
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
    if (targ->is_sync == 0)
    {
      produce (targ->buff[1], sampleDataAsString, strlen(sampleDataAsString)+1);
    }
    else if (targ->is_sync == 1)
    {
      slotwrite (targ->buff[1], sampleDataAsString, strlen(sampleDataAsString)+1);
    }
  }
  //EOF ending signal 
  if (targ->is_sync == 0)
  {
    produce (targ->buff[1], "EOF", strlen("EOF")+1);
  }
  else if (targ->is_sync == 1)
  {
    slotwrite (targ->buff[1], "EOF", strlen("EOF")+1);
  }
  //printf("Complete Process 2\n");
  pthread_exit(NULL);
}
