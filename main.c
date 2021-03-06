#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include "fifo.h"
#include "config.h"
#include "port.h"

extern int  portInit(void);
extern int  menuInit();
extern void menuLoop();
extern int  menuAddItem(char *name, int (*cbFn)(int, char *argv[]) , char *help);
extern void* thMmeTx(void *ptr);
extern void* thMmeRx(void *ptr);
extern void* thS1uTx(void *ptr);
extern void* thSgiRx(void *ptr);
extern void* thSgiTx(void *ptr);
extern void* thS1uRx(void *ptr);
extern void* thStats(void *ptr);

int cbS11(int argc, char *argv[] );

int cbUeStart(int argc, char *argv[] );
int cbUeStop(int argc, char *argv[] );

int cbSgiStart(int argc, char *argv[] );
int cbSgiStop(int argc, char *argv[] );

int cbGetStats(int argc, char *argv[] );

msgFifo_t g_msg_qs[16];
sim_port_t g_ports[4];

int main(void){	
    int i;
    int iCore[16];
    cpu_set_t cpu1;

    pthread_t pt_mmeTx;
    pthread_t pt_mmeRx;

    pthread_t pt_s1uTx;
    pthread_t pt_sgiRx;
    pthread_t pt_sgiTx;
    pthread_t pt_s1uRx;

    pthread_t pt_stats;

    for (i = 0; i < 16; i++) {
        iCore[i] = 17+i;
    }
    CPU_ZERO(&cpu1);
    CPU_SET(16, &cpu1);
    printf("main cpu %d\n", sched_getcpu());
      
    i = sched_setaffinity(0, sizeof(cpu_set_t), &cpu1);
    printf("main cpu %d\n", sched_getcpu());

    portInit();

    //init msg queues
    for (i = 0; i < 16; i++) {
       initFifo(&g_msg_qs[i]);
    }
    //start stats threads
    if(pthread_create(&pt_mmeTx, NULL, &thMmeTx, (void *) &iCore[0])) {
       printf("pthread_create failed thMmeTx \n");
       exit(1);
    }


    if(pthread_create(&pt_mmeRx, NULL, &thMmeRx, (void *) &iCore[1])) {
       printf("pthread_create failed thMmeRx \n");
       exit(1);
    }

    if(pthread_create(&pt_s1uTx, NULL, &thS1uTx, (void *) &iCore[2])) {
       printf("pthread_create failed thS1uTx \n");
       exit(1);
    }


    if(pthread_create(&pt_sgiRx, NULL, &thSgiRx,(void *) &iCore[3])) {
       printf("pthread_create failed thSgiRx\n");
       exit(1);
    }


    if(pthread_create(&pt_sgiTx, NULL, &thSgiTx, (void *) &iCore[4])) {
       printf("pthread_create failed thSgiTx\n");
       exit(1);
    }
 
    if(pthread_create(&pt_s1uRx, NULL, &thS1uRx,(void *) &iCore[5])) {
       printf("pthread_create failed thS1uRx\n");
       exit(1);
    }

     if(pthread_create(&pt_stats, NULL, &thStats,(void *) &iCore[6])) {
       printf("pthread_create failed thStats\n");
       exit(1);
    }

    menuInit();
    menuAddItem("s11", cbS11, "send s11 setup");

    menuAddItem("startue", cbUeStart, "send s1u Start UE traffic");
    menuAddItem("stopue", cbUeStop, "send s1u1 stop UE traffic");
    menuAddItem("startsgi", cbSgiStart, "send sgi Start UE traffic");
    menuAddItem("stopsgi", cbSgiStop, "send sgi Stop UE traffic");
    menuAddItem("stats", cbGetStats, "get stats");
    menuLoop();

	return 0;
}


int cbS11(int argc, char *argv[] ){

    cli_msg_t cliMsgWork;
 


    cliMsgWork.cmd = REQ_START;
//    if (argc) {
//        burst = atoi(argv[1]);
//        printf("  burts %d\n", burst);
//    }

    while(enqueueFifo(&g_msg_qs[msgq_cli_to_mmeTx], &cliMsgWork));

    return 1;

}

int cbGetStats(int argc, char *argv[] ){

    cli_msg_t cliMsgWork;
 


    cliMsgWork.cmd = REQ_START;
//    if (argc) {
//        burst = atoi(argv[1]);
//        printf("  burts %d\n", burst);
//    }

    while(enqueueFifo(&g_msg_qs[msgq_cli_to_stats], &cliMsgWork));

    return 1;

}

int cbUeStart(int argc, char *argv[] ){

    cli_msg_t cliMsgWork;
 


    cliMsgWork.cmd = REQ_START;
    if (argc) {
        cliMsgWork.arg1 = atoi(argv[1]);
    }

    while(enqueueFifo(&g_msg_qs[msgq_cli_to_s1uTx], &cliMsgWork));

    return 1;

}

int cbUeStop(int argc, char *argv[] ){

    cli_msg_t cliMsgWork;
 


    cliMsgWork.cmd = REQ_STOP;
//    if (argc) {
//        burst = atoi(argv[1]);
//        printf("  burts %d\n", burst);
//    }

    while(enqueueFifo(&g_msg_qs[msgq_cli_to_s1uTx], &cliMsgWork));

    return 1;

}

int cbSgiStart(int argc, char *argv[] ){

    cli_msg_t cliMsgWork;
 


    cliMsgWork.cmd = REQ_START;
    if (argc) {
        cliMsgWork.arg1 = atoi(argv[1]);
    }

    while(enqueueFifo(&g_msg_qs[msgq_cli_to_sgiTx], &cliMsgWork));

    return 1;

}


int cbSgiStop(int argc, char *argv[] ){

    cli_msg_t cliMsgWork;
 


    cliMsgWork.cmd = REQ_STOP;
//    if (argc) {
//        burst = atoi(argv[1]);
//        printf("  burts %d\n", burst);
//    }

    while(enqueueFifo(&g_msg_qs[msgq_cli_to_sgiTx], &cliMsgWork));

    return 1;

}

