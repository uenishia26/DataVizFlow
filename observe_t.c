#include "libobjdata.h" //Add header file
#define MAX_PAIRS 50 //Number of nameValuePairs 

void addNameValuePair(int index, NameValuePair *pairs, char *name, char *value)
{
  strncpy(pairs[index].name, name, MAX_DATA_LENGTH);
  pairs[index].name[strlen(name)] = '\0';
  strncpy(pairs[index].value, value, MAX_DATA_LENGTH);
  pairs[index].name[strlen(name)] = '\0';
}

void *observe(void *arg)
{
  thread_arg *targ = (thread_arg *) arg; //get the arguments sent to thread
    
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
    if (targ->is_sync == 0)
    {
      produce (targ->buff[0], nameValCombined, strlen(nameValCombined)+1);
    }
    else if (targ->is_sync == 1)
    {
      slotwrite (targ->buff[0], nameValCombined, strlen(nameValCombined)+1);
    }
  }
  
  //This is to indicate termination
  if (targ->is_sync == 0)
  {
    produce (targ->buff[0], "EOF", strlen("EOF")+1);
  }
  else if (targ->is_sync == 1)
  {
    slotwrite (targ->buff[0], "EOF", strlen("EOF")+1);
  }
  //printf("Completed Process 1\n");
  pthread_exit(NULL);
}
