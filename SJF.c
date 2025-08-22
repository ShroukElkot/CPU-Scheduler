#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>

#define MSG_QUEUE_KEY 123456
typedef struct {
    int ID;
    int ArrTime;
    int RunTime;
    int priority;
} Process;
typedef struct Node {
    Process *data;
    struct Node *next;
} Node;

// This is the same struct as in your sender program
typedef struct {
    long mType;
    Process mProcess;
} Message;
typedef enum{
    READY, STARTED, RESUMED, STOPED, FINISHED
} State;

typedef struct PCB {
    Process process;
    State state;
    int RemTime, WaitTime;
    struct PCB *next;
} PCB;

// Ready Queue
typedef struct {
    PCB *front;
    PCB *rear;
} ReadyQueue;
// Enqueue a process into the ready queue based on priority (SJF)
void enqueue_SJF(ReadyQueue *queue, Process process) {
    PCB *newPCB = (PCB *)malloc(sizeof(PCB));
    newPCB->process = process;
    newPCB->state = READY;
    newPCB->RemTime = process.RunTime;
    newPCB->WaitTime = 0;
    newPCB->next = NULL;

    if (queue->front == NULL) {
        // Queue is empty
        queue->front = newPCB;
        queue->rear = newPCB;
    } else {
        // Find the position to insert the process based on priority (burst time)
        PCB *temp = queue->front;
        PCB *prev = NULL;
        while (temp != NULL && temp->process.RunTime < process.RunTime) {
            prev = temp;
            temp = temp->next;
        }
        if (prev == NULL) {
            // Insert at the front
            newPCB->next = queue->front;
            queue->front = newPCB;
        } else {
            // Insert after prev
            prev->next = newPCB;
            newPCB->next = temp;
        }
        if (temp == NULL) {
            // Update rear if process is inserted at the end
            queue->rear = newPCB;
        }
    }
}

// Dequeue a process from the ready queue
PCB* dequeue(ReadyQueue *queue) {
    if (queue->front == NULL) {
        // Queue is empty
        return NULL;
    }
    PCB *process = queue->front;
    queue->front = process->next;
    if (queue->front == NULL) {
        // Queue is now empty
        queue->rear = NULL;
    }
    return process;
}

// Function to display the contents of the ready queue
void displayReadyQueue(ReadyQueue *queue) {
    printf("Ready Queue: ");
    PCB *temp = queue->front;
    while (temp != NULL) {
        printf("P%d ", temp->process.ID);
        temp = temp->next;
    }
    printf("\n");
}


int countProcesses(ReadyQueue *queue)
{
    int count =0;
    PCB* current = queue->front;
    while(current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;

}

int countRemaining(ReadyQueue *queue)
{
    int count =0;
    PCB* current = queue->front;
    while(current != NULL)
    {
        if(current->state == READY)
        {
            count++;
        }
        current = current->next;
    }
    return count;
}

void initializeReadyQueue(ReadyQueue *queue) {
    queue->front = NULL;
    queue->rear = NULL;
}

// Function to receive and print a process
/*void receiveProcess(ReadyQueue *readyQueue, int msgQueueId) {
    Message msg;
    while (msgrcv(msgQueueId, &msg, sizeof(msg.mProcess), 0, !IPC_NOWAIT) != -1) {
        printf("Received process with ID: %d, Arrival Time: %d, Run Time: %d, Priority: %d\n",
               msg.mProcess.ID, msg.mProcess.ArrTime, msg.mProcess.RunTime, msg.mProcess.priority);
        // Enqueue the received process into the ready queue
        Process newProcess = msg.mProcess; // Copying the received process data
        enqueue_SJF(readyQueue, newProcess);
        //printf("Ready queue is:\n");
        displayReadyQueue(readyQueue);
    }
    if (errno != ENOMSG) {
        perror("Failed to receive message");
        exit(EXIT_FAILURE);
    }
}*/
void receiveProcess(ReadyQueue *queue, int msgQuID)
{
    Message msgProcess;
    int rcv = msgrcv(msgQuID, (void*)&msgProcess, sizeof(msgProcess.mProcess), 0, !IPC_NOWAIT);
    if (rcv == -1)
    {
        perror("Error in receiving");
    }
    else
    {
        printf("Message Recieved ");
        printf("Received process with ID: %d, Arrival Time: %d, Run Time: %d, Priority: %d\n",
               msgProcess.mProcess.ID, msgProcess.mProcess.ArrTime, msgProcess.mProcess.RunTime, msgProcess.mProcess.priority);
    enqueue_SJF(queue, msgProcess.mProcess);
    displayReadyQueue(queue);
}
}
void logFILE(FILE *logfile, int time, int ID, State state, int arrtime, int runtime, int remainingProcesses, int waitingProcesses)
{
    char *stateTostring;
    switch(state)
    {
        case READY:
        stateTostring = "ready";
        break;
        case STARTED:
        stateTostring = "started";
        break;
        case RESUMED:
        stateTostring = "resumed";
        break;
        case STOPED:
        stateTostring = "stopped";
        break;
        case FINISHED:
        stateTostring = "finished";
        break;
    }
    fprintf(logfile, "At time %d process %d state %s arr %d total %d remain %d wait %d", time, ID, stateTostring, arrtime, runtime, remainingProcesses, waitingProcesses);
}


double calcAvgWaitingTime(ReadyQueue *queue)
{
    int total_processes = countProcesses(queue);
    double totalWaitingTime  = 0.0;
    if(total_processes == 0)
    {
        return 0.0;
    }
    PCB *currProcess = queue->front;
    while(currProcess!=NULL)
    {
        int waitingTime = currProcess->WaitTime;
        totalWaitingTime += waitingTime;
        currProcess = currProcess->next;
    }
    double Avg = totalWaitingTime/total_processes;

    return Avg;
}

double WTAtime(int startTime, int finalTime)
{
    double WTA = finalTime- startTime;
    return WTA;
}

typedef struct
{
    double AvgWTA;
    double StdWTA;
} statisics;

statisics executeProcesses(ReadyQueue *queue, FILE *logfile)
{
    int totalProcesses = countProcesses(queue);
    int totalWTA_time = 0;
    int totalWTA_squared = 0;
    int currentTime = 0;
    while(queue->front != NULL)
    {
       
        int remaining_processes = countProcesses(queue);
        int waitingProcesses = countRemaining(queue);
        PCB *currProcess = dequeue(queue);
        printf("current process %d/n" , currProcess->process.ID);
        if(currProcess != NULL)
        {
            currProcess->state = STARTED;
            logFILE(logfile, getClk(), currProcess->process.ID, currProcess->state, currProcess->process.ArrTime, totalProcesses, remaining_processes, waitingProcesses);

            pid_t pid = fork();
                if(pid == -1)
                {
                    perror("error in forking");
                    exit(EXIT_FAILURE);
                }
                else if(pid == 0)
                {
                    printf("Process %d with ArrivalTime %d,RumTime %d, and Priority %d has the parameters .\n", currProcess->process.ID, currProcess->process.ArrTime, currProcess->process.RunTime, currProcess->process.priority);
                    exit(EXIT_SUCCESS);
                }

            int timeTostartFrom = getClk();
            int remaining = currProcess->process.RunTime;

            //while (getClk() - timeTostartFrom < remaining)
            //{
                //printf("Process %d executing... Remaining Time: %d\n", currProcess->process.ID, remaining-(getClk()-timeTostartFrom));
            //}
            //sleep(currProcess->process.RunTime);
            int endTime = getClk();

            int WTA_time = WTAtime(timeTostartFrom, endTime);
            totalWTA_time += WTA_time;
            totalWTA_squared += WTA_time * WTA_time;

            //recieving signal from process to terminate
            currProcess->state = FINISHED;
            logFILE(logfile, getClk(), currProcess->process.ID, currProcess->state, currProcess->process.ArrTime, totalProcesses, remaining_processes, waitingProcesses);
            free(currProcess);
        }

    }
    statisics stat;
    double avgWTA = totalWTA_time/totalProcesses;
    double stdWTA = sqrt((totalWTA_squared - (2 * totalWTA_time * avgWTA) + (avgWTA * avgWTA * totalProcesses)) / totalProcesses);
    stat.AvgWTA = avgWTA;
    stat.StdWTA = stdWTA;
    return stat;
}


//int msgQueueId = 0;
int Algoswitch = 0;


int main(int argc, char * argv[]) {
    // Initialize clock
    initClk();
    FILE *logfile = fopen("scheduler.log", "w");
    if (!logfile)
    {
        perror("Can't open the file");
        exit(EXIT_FAILURE);
    }

    ReadyQueue sjfQueue;
    initializeReadyQueue(&sjfQueue);
    // Create or connect to the message queue
    int msgQueueId = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (msgQueueId == -1) {
        perror("Receiver: Failed to create/connect to the message queue");
        exit(EXIT_FAILURE);
    }
    // Start receiving messages
    printf("Receiver started. Waiting for processes...\n");
        // Receive messages from the message queue
    receiveProcess(&sjfQueue, msgQueueId);
    statisics schedulerData;
    schedulerData = executeProcesses(&sjfQueue, logfile);
    printf("ayyyy message/n");
    double CPU_utilization = 100.0;
    double avgWaiting = calcAvgWaitingTime(&sjfQueue);

    FILE *perfFile = fopen("scheduler.perf", "w");
    if (!perfFile)
    {
        printf("Can't open the file");
    }
    fprintf(perfFile, "CPU utilization = %0.2f%%\n", CPU_utilization);
    fprintf(perfFile, "Avg WTA = %0.2f\n", schedulerData.AvgWTA);
    fprintf(perfFile, "Avg Waiting = %0.2f\n", avgWaiting);
    fprintf(perfFile, "Std WTA = %0.2f\n", schedulerData.StdWTA);
    fclose(perfFile);



    fclose(logfile);


    // Clean up before exiting
    destroyClk(true);
    // Add any necessary clean up for the message queue and ready queue here

    return 0;
}