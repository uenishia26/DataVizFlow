// Online C compiler to run C program online
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdlib.h>
#include <sys/shm.h> //For generating keys in 
#include <semaphore.h> 
#include <string.h> //For string compare
#include <sys/wait.h>
#include <ctype.h> //For is digit 
#define MAX_SLOT_LENGTH 1000//Max lenght of each slot of the buffer

char *typeBuffer;
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

//initalizing slot buffer
void initalizeSlotBuff(ringBuffer *sb)
{
  sb->latest = 0;
  sb->reading = 0;
  sb->slot[0] = 0;
  sb->slot[1] = 0;
}

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
    //printf("%s\n", rb->buffer + rb->head*MAX_SLOT_LENGTH); 
    rb->head = ((rb->head+1) % rb->bufferSize); 
    sem_post(&rb->emptySlots); 
}

int isNumber(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) 
    {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    
    int options; 
    int numSlots = 1;//Default is 1 slot unless changed 
    char *programList [1000]; 
    int programIndex = 0; 
    int shmid; 
    int argn = 1; //Default 

    
    //Command line parsing for programs and optional argn 
    int index = 1;
    while (index < argc) {
        if (strncmp(argv[index], "-p", 2) == 0 && index + 1 < argc) {
            programList[programIndex++] = argv[index + 1];
            index += 2; // Skip next as it's part of -p
        } else if (isNumber(argv[index])) {
            argn = atoi(argv[index]);
            index++;
            break; // Found argn, move to next part
        } else {
            // Not a -p option or a number, break and handle other options
            break;
        }
    }
    optind = index; //Setting getops starting indx 

    //Command line parsing for the sync type and the size of the buffer 
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

    //printf("argn %d\n", argn); 
    //printf("Buffer type %s\n", typeBuffer);
    //printf("size %d\n", numSlots); 
    
    //Initalization depending on async or sync
    int p1p2memoryID; 
    int p2p3memoryID;
    ringBuffer *rb;
    ringBuffer *rb2;
    ringBuffer *sb;
    ringBuffer *sb2;
    if (strcmp(typeBuffer, "async") == 0)
    {
      p1p2memoryID = getMemoryId(4);
      p2p3memoryID = getMemoryId(4);
      sb= (ringBuffer *)createSharedMemory(p1p2memoryID);
      sb2 = (ringBuffer *)createSharedMemory(p2p3memoryID);
      initalizeSlotBuff(sb);
      initalizeSlotBuff(sb2);
    }
    else
    {
      p1p2memoryID = getMemoryId(numSlots);
      p2p3memoryID = getMemoryId(numSlots);
      rb= (ringBuffer *)createSharedMemory(p1p2memoryID);
      rb2 = (ringBuffer *)createSharedMemory(p2p3memoryID);
      initalizeRingBuff(rb,numSlots);
      initalizeRingBuff(rb2, numSlots);
    }

    //Convert memoryID into STR to pass to P1
    char p1p2shmidStr[12]; 
    sprintf(p1p2shmidStr, "%d", p1p2memoryID);
    //printf("ID1: %s", p1p2shmidStr); 

    char p2p3shmidStr[12]; 
    sprintf(p2p3shmidStr, "%d", p2p3memoryID); 

    //convert argn to pass to P3
    char argnStr[12]; 
    sprintf(argnStr, "%d", argn);

    //convert typeBuffer to pass to P1, P2, P3
    char typeStr[12];
    sprintf(typeStr, "%s", typeBuffer);
 
    //Creating the path names for running the child processes 
    char p1Path [50];
    sprintf(p1Path, "./%s", programList[0]); 
    char p2Path [50];
    sprintf(p2Path, "./%s", programList[1]); 
    char p3Path [50]; 
    sprintf(p3Path, "./%s", programList[2]); 


    //Starting child process and change process image to P1 
    int p1 = fork(); 
    if (p1 == 0)
    {
      execl(p1Path, programList[0], p1p2shmidStr, typeStr, NULL);
        perror("execl P1 failed"); 
        exit(EXIT_FAILURE); 
    }
    

    //Start child process and change proce image to P2
    int p2 = fork(); 
    if(p2 == 0)
    {
      execl(p2Path, programList[1], p1p2shmidStr, p2p3shmidStr, typeStr, NULL); 
      perror("execl P2 failed"); 
      exit(EXIT_FAILURE); 
    }

    int p3 = fork(); 
    if(p3 == 0)
    {
      execl(p3Path, programList[2], p2p3shmidStr, argnStr, typeStr, NULL);
    }

    //Waiting for the parent to exit 
    if(p1 > 0)
      waitpid(p1, NULL, 0); 
    if(p2 > 0)
      waitpid(p2, NULL, 0); 
    if(p3 > 0)
      waitpid(p3, NULL, 0); 
    
    
    char line[1000]; 
    FILE *file = fopen("dataFile.txt", "r"); 
    while(fgets(line, sizeof(line), file))
    {
        printf("%s", line); 
    }
    fclose(file); 
    //printf("Program has finished executing: Removing shared memory segements...\n"); 
    shmctl(p1p2memoryID, IPC_RMID, NULL); 
    shmctl(p2p3memoryID, IPC_RMID, NULL); 


    
    return 0; 


}
