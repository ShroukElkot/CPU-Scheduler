#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>

struct msgbuf
{
    long mtype;
    char mtext[64];

};

int main()
{
    int clock;
    struct msgbuf message;
    int rec_val;
    key_t keUpDisk, keyDownDisk, keyUpUser, keyDownUser;
    //The kernel process communicates with the user process 
    keyUpUser = ftok("kernel", 1);
    keyDownUser = ftok("kernel", 2);
    int UpQueue_user = msgget(keyUpUser, IPC_CREAT | 0644);
    int DownQueue_user = msgget(keyDownUser,IPC_CREAT | 0644);
    if (UpQueue_user == -1 || DownQueue_user == -1) {
        perror("error in message queues");
        exit(EXIT_FAILURE);
    }
    printf("The user Up Queue created with ID : %d\n", UpQueue_user);
    printf("The user Down Queue created with ID : %d\n", DownQueue_user);

    msgrcv(UpQueue_user, &message, sizeof(message.mtext),0,!IPC_NOWAIT); //tag 0 means get the first message, !NOWAIT means wait till a message recieved. 
    msgsnd(DownQueue_user, &message, sizeof(message.mtext), !IPC_NOWAIT);

    //The kernel process communicates with the disk process

    keyUpDisk = ftok("kernel",3);
    keyDownDisk = ftok("kernel",4);

    int UpQueue_disk = msgget(keyUpDisk,IPC_CREAT | 0644);
    int DownQueue_disk = msgget(keyDownDisk,IPC_CREAT | 0644);
    if (UpQueue_disk == -1 || DownQueue_disk == -1) {
        perror("error in message queues");
        exit(EXIT_FAILURE);
    } 
    printf("The disk Up Queue created with ID : %d\n", UpQueue_disk);
    printf("The disk Down Queue created with ID : %d\n", DownQueue_disk);


    msgrcv(UpQueue_disk, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
    msgget(DownQueue_disk, &message, sizeof(message.mtext), !IPC_NOWAIT);


    return 0;

};