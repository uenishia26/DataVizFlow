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

NameValuePair parseSampleDataStr(char *sampleData, int argn)
{
  NameValuePair eofCheck; 
  if(strcmp(sampleData, "EOF")==0)
  {
    strncpy(eofCheck.name, "EOF", MAX_DATA_LENGTH); 
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
    else
    {
      whichNameValuePair++; 
    }
  }
}

void *tapplot(void *arg)
{
  printf("In process 3\n");
  thread_arg *targ = (thread_arg *) arg; //get the arguments sent to thread

  //Create/Open the file in append mode 
  FILE *file = fopen("dataFile.txt", "w"); 
  if(file == NULL)
  {
    perror("Error creating/opening the file"); 
    exit(1); 
  }

  //Set up gnuplot by including one data input so gnuplot can determine a range 
  NameValuePair tempNVP;
  if (targ->is_sync == 0)
  {
    tempNVP = parseSampleDataStr(consume (targ->buff[1]), targ->argn);
  }
  else if (targ->is_sync == 1)
  {
    tempNVP = parseSampleDataStr(slotread (targ->buff[1]), targ->argn);
  }
  fprintf(file, "%d %s\n", 1, tempNVP.value);
  fflush(file);
  system("gnuplot 'live_plot.gp' &"); //Allows for gnuplot to run in the background 

  int x = 0;  //This is sampleNumber 
  while(true)
  {
    NameValuePair tempNVP;
    if (targ->is_sync == 0)
    {
      tempNVP = parseSampleDataStr(consume (targ->buff[1]), targ->argn);
    }
    else if (targ->is_sync == 1)
    {
      tempNVP = parseSampleDataStr(slotread (targ->buff[1]), targ->argn);
    } 

    if (strcmp(tempNVP.name, "") != 0)
    {
      //Exit the loop as soon as we reach a EOF signal 
      if(strcmp(tempNVP.name, "EOF") == 0)
	break;
    
      fprintf(file, "%d %s\n", x+1, tempNVP.value); 
      fflush(file);   
      x++;
    }
    //for (int j = 0; j < 1E8; j++); /* Add some delay. */
    //sched_yield ();/* Allow another thread to run. */
  }
  
  fclose(file);
  printf("Completed Process 3\n");
  pthread_exit(NULL);
}
