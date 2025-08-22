#include "headers.h"
#include <math.h>

typedef struct {
    int ID;
    int ArrTime;
    int RunTime;
    int priority;
} Process;

typedef struct {
    long mtype;
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

int msgQuID = 0;
int Algoswitch = 0;

// Initialize an empty ready queue
void initializeReadyQueue(ReadyQueue *queue) {
    queue->front = NULL;
    queue->rear = NULL;
}

// Enqueue a process into the ready queue based on priority (SJF)
void enqueue_SRTN(ReadyQueue *queue, Process process) {
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
        // Find the position to insert the process based on remaining time
        PCB *temp = queue->front;
        PCB *prev = NULL;
        while (temp != NULL && temp->RemTime < newPCB->RemTime) {
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

void rcvProcesses(ReadyQueue *queue, int msgQuID)
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

    enqueue_SRTN(queue, msgProcess.mProcess);

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
    PCB* currProcess = NULL; 
    int timeTostartFrom = 0;
    while(1)
    {
        int remaining_processes = countProcesses(queue);
        int waitingProcesses = countRemaining(queue);

        if(currProcess == NULL)
        {
            currProcess = dequeue(queue);
            if(currProcess == NULL) //The queue is empty
            {
                break;
            }
            currProcess->state = STARTED;
            timeTostartFrom = getClk();

        }
        int remaining = currProcess->RemTime - (getClk() - timeTostartFrom);
        while(remaining>0)
        {
            // if there is a process recieved has remaining time less than the one in execution
            rcvProcesses(queue, msgQuID);
            PCB* temp_process = queue->front;
            if (temp_process != NULL && temp_process->RemTime<remaining)
            {
                //enqueue the old process
                enqueue_SRTN(queue, currProcess->process);
                currProcess->state = STOPED;
                logFILE(logfile, getClk(), currProcess->process.ID, currProcess->state, currProcess->process.ArrTime, totalProcesses, remaining_processes, waitingProcesses);
                //dequeue the new process
                currProcess = dequeue(queue);
                currProcess->state =    STARTED;
                timeTostartFrom = getClk();
                remaining = currProcess->RemTime;
            }
            remaining = currProcess->RemTime - (getClk() - timeTostartFrom);
            //if process finished execution
            if (remaining <= 0)
            {
                break;
            }
        }

        int endTime = getClk();

        int WTA_time = WTAtime(currProcess->process.ArrTime, endTime);
        totalWTA_time += WTA_time;
        totalWTA_squared += WTA_time * WTA_time;

        //recieving signal from process to terminate
        currProcess->state = FINISHED;
        logFILE(logfile, getClk(), currProcess->process.ID, currProcess->state, currProcess->process.ArrTime, totalProcesses, remaining_processes, waitingProcesses);
        free(currProcess);
        currProcess = NULL;


    }
    statisics stat;
    double avgWTA = (double)totalWTA_time/totalProcesses;
    double stdWTA = sqrt((totalWTA_squared - (2 * totalWTA_time * avgWTA) + (avgWTA * avgWTA * totalProcesses)) / totalProcesses);
    stat.AvgWTA = avgWTA;
    stat.StdWTA = stdWTA;
    return stat;
}



int main(int argc, char * argv[])
{
    initClk();
    FILE *logfile = fopen("scheduler.log", "w");
    if (!logfile)
    {
        perror("Can't open the file");
        exit(EXIT_FAILURE);
    }

    ReadyQueue SRTNQueue;
    initializeReadyQueue(&SRTNQueue);
    //inter process communication
    msgQuID = msgget(0001, IPC_CREAT | 0644);
    if(msgQuID == -1)
    {
        perror("error creating the msgQ");
        exit(1);
    }

    rcvProcesses(&SRTNQueue, msgQuID);

    statisics schedulerData;
    schedulerData = executeProcesses(&SRTNQueue, logfile);
    double CPU_utilization = 100.0;
    double avgWaiting = calcAvgWaitingTime(&SRTNQueue);

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


    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}





