#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MSG_QUEUE_KEY 123456
// Structure to represent a process
typedef struct {
    int ID;
    int ArrTime;
    int RunTime;
    int priority;
} Process;

// Structure to represent a node in the process queue
typedef struct Node {
    Process *data;
    struct Node *next;
} Node;

// Structure to represent the process queue
typedef struct {
    Node *front;
    Node *rear;
} ProcessQueue;

// Function to initialize the process queue
ProcessQueue* initProcessQueue() {
    ProcessQueue *queue = (ProcessQueue*)malloc(sizeof(ProcessQueue));
    if (queue == NULL) {
        perror("Error: Memory allocation failed.");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

// Function to enqueue a process into the process queue
void enqueue(ProcessQueue *queue, Process *process) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Error: Memory allocation failed.");
        exit(EXIT_FAILURE);
    }
    newNode->data = process;
    newNode->next = NULL;
    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}
// Dequeue a process from the process queue
void dequeue(ProcessQueue *queue) {
    if (queue->front == NULL) {
        // Queue is empty
        return;
    }
    Node *temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        // Queue is now empty
        queue->rear = NULL;
    }
    free(temp->data);
    free(temp);
}
// Function to print the contents of the process queue
void printProcessQueue(ProcessQueue *queue) {
    Node *current = queue->front;
    printf("Process Queue:\n");
    while (current != NULL) {
        printf("ID: %d, Arrival Time: %d, Run Time: %d, Priority: %d\n",
               current->data->ID, current->data->ArrTime,
               current->data->RunTime, current->data->priority);
        current = current->next;
    }
}
pid_t SchedulerPid;
pid_t ClockPid;
int MsgQueueId;

void createmsgque() {
    MsgQueueId = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (MsgQueueId == -1) {
        perror("PG: IPC init failed");
        raise(SIGINT);
    }
    printf("message queue created sucsessfully\n");
}
// Function to read the file and enqueue each line as a process in the process queue
void readAndEnqueue(const char *filename, ProcessQueue *queue) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error: Unable to open the file.");
        exit(EXIT_FAILURE);
    }

    char line[100]; // Assuming maximum line length is 100 characters
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') // Skip empty lines
            continue;

        Process *process = (Process*)malloc(sizeof(Process));
        if (process == NULL) {
            perror("Error: Memory allocation failed.");
            exit(EXIT_FAILURE);
        }

        // Parse line and fill process attributes
        sscanf(line, "%d %d %d %d", &process->ID, &process->ArrTime, &process->RunTime, &process->priority);
       
        enqueue(queue, process);
    }

    fclose(file);
}
void ExecuteClock() {
    ClockPid = fork();
    while (ClockPid == -1) {
        perror("PG: *** Error forking clock");
        printf("PG: *** Trying again...\n");
        ClockPid = fork();
    }
    if (ClockPid == 0) {
        printf("PG: *** Clock forking done!\n");
        printf("PG: *** Executing clock...\n");
        char *argv[] = {"clkout", NULL};
        execv("clkout", argv);
        perror("PG: *** Clock execution failed");
        exit(EXIT_FAILURE);
    }

}
//void clearResources(int);
void ExecuteScheduler(int alg) {
    SchedulerPid = fork();
    printf("\n----------------------------------------------%d\n", alg);
    while (SchedulerPid == -1) {
        perror(" Error forking scheduler");
        printf("Trying again...\n");
        SchedulerPid = fork();
    }
    if (SchedulerPid == 0) {
        char *argv[3];
        
        printf(" Scheduler forking done!\n");
        printf(" Executing scheduler...\n");
       
        argv[1] = NULL;
        switch (alg) {
            case 1:
                argv[0] = "sjfout";
                printf("the sjf algorithm will execute");
                execv("sjfout", argv);
                break;
            default:
                break;
        }
        perror("PG: *** Scheduler execution failed");
        exit(EXIT_FAILURE);
        exit(EXIT_SUCCESS);
        
    }
}
typedef struct 
{
    long mType;
    Process mProcess;
}Message;

void SendProcess(Process *Process, int MsgQueueId){
    Message msg;
    msg.mType = 1;
    msg.mProcess = *Process;
    printf(" Sending process with id %d to scheduler...\n", Process->ID);
    int val = msgsnd(MsgQueueId, (void *) &msg, sizeof(msg.mProcess),IPC_NOWAIT);
    if (val == -1) {perror("Failed sending message :(");}
    else{printf("\n Message sent SUCCESSFULLY :D \n");
    printf("okk/n");
    printf("sent process with ID: %d, ArriTime: %d , runtime: %d , periority: %d/n", Process->ID, Process->ArrTime, Process->RunTime , Process->priority);
    }
}


int main(int argc, char * argv[])
{
    initClk();
    
    //signal(SIGINT, clearResources);
    ProcessQueue *queue = initProcessQueue();
    readAndEnqueue("input.txt", queue);
    printProcessQueue(queue);
   printf("enter the schedueling algorithm you want");
    int chosen;
   scanf("%d" , &chosen);
   printf("you choose algo: %d/n" , chosen);
   createmsgque();
   //ExecuteClock();
   ExecuteScheduler(chosen); 
   
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
      while (queue->front != NULL) 
      {
         int Time_Now = getClk();
         if(queue->front->data->ArrTime == Time_Now)
            {
                SendProcess(queue->front->data , MsgQueueId);
                dequeue(queue);
            }
            else
            {
                sleep(1);
                 //printf("no peocesses match time /n");
            }
            
                //send SIGUSR1 to the scheduler
        
        //if (is_time){
            //kill(SchedulerPid, SIGUSR1);}
      }

    destroyClk(true); 
     
    }
    
     






