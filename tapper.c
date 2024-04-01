// Online C compiler to run C program online
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <sys/wait.h>
#define MAX_SLOT_LENGTH 32 //Max lenght of each slot of the buffer 

typedef struct 
{
  sem_t filledSlots;
  sem_t emptySlots;    
  int head; 
  int tail; 
  int bufferSize; 
  char buffer[];  
}ringBuffer; 

void initalizeRingBuff(ringBuffer *rb, int size)
{
  sem_init(&rb->filledSlots, 1, 0); 
  //Semaphore, communication between two processes, intStartPoint
  sem_init(&rb->emptySlots, 1, size);
  rb->head = 0; 
  rb->tail = 0; 
  rb->bufferSize = size; 
}

size_t getTotalSize(int bufferSize)
{
    size_t totalSize = (MAX_SLOT_LENGTH * bufferSize) + sizeof(ringBuffer); 
    return (totalSize); 
}

int getMemoryId(int bufferSize)
{
    int memoryID = shmget(IPC_PRIVATE, getTotalSize(bufferSize), 0644); 
    if(memoryID == -1)
    {
        printf("Fail to create key"); 
        exit(EXIT_FAILURE); 
    }
    return memoryID; 
}

void *createSharedMemory(int memoryID)
{
    void *temp = shmat(memoryID, NULL, 0); 
    if(temp == (void *)-1)
    {
        printf("Fail to attached Memory"); 
        exit(EXIT_FAILURE); 
    }
    return temp; 
}
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
    printf("%s\n", rb->buffer + rb->head*MAX_SLOT_LENGTH); 
    rb->head = ((rb->head+1) % rb->bufferSize); 
    sem_post(&rb->emptySlots); 
}

int main(int argc, char *argv[])
{
    
    int options; 
    int numSlots = 1;//Default is 1 slot unless changed 
    char *typeBuffer; 
    char *programList [1000]; 
    int programIndex = 0; 
    int shmid; 

    //Parsing command line arguments 
    for(int index = 1; index < argc; index++)
    {
        if(strncmp(argv[index], "-p", 2) == 0)
        {
            programList[programIndex] = argv[++index]; 
            programIndex++; 
        }
        else
        {   //Set getopt to start from this index 
            optind = index; 
            break; 
        }
    }

    while((options=getopt(argc, argv, "b:s:"))!=-1) 
    {
        switch(options)
        {
            case 'b':
                typeBuffer = optarg; 
                break; 

            case 's':
                numSlots = atoi(optarg); 
                break; 

            case '?':
                printf("Unknown option"); 
                break; 
        }
    }  
    int memoryID = getMemoryId(numSlots); 
    ringBuffer *rb= (ringBuffer *)createSharedMemory(memoryID); 
    initalizeRingBuff(rb,numSlots); 

    //Convert memoryID into STR to pass to P1
    char shmidStr[12]; 
    sprintf(shmidStr, "%d", memoryID); 
    
  

    //Starting child process and change process image to P1 
    int p1 = fork(); 
    if (p1 == 0)
    {
        execl("/home/anamu/Desktop/CS410Assign2/P1", "P1", shmidStr, NULL);
    }
    else
    {
        wait(NULL); 
    }
    
    //Start child process and change proce image to P2
    int p2 = fork(); 
    if(p2 == 0)
    {
        execl("/home/anamu/Desktop/CS410Assign2/P2", "P2", shmidStr, NULL); 
    }
    else
    {
        wait(NULL); 
    }

    


/*
    char strShmid[20]; 
    sprintf(strShmid, "%d", memoryID); 
    int p1 = fork(); 
    if(p1 == 0) //First Process
    {
        if(execl("/home/anamu/Desktop/CS410Assign2/write", "write", strShmid, NULL) == -1)
            perror("Error w/h execl");
        
    }
    else //Wait until the parent process terminates 
    {
        wait(NULL); 
    }    
*/
    
    return 0; 


}
