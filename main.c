#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "fifo.h"
#include "config.h"
#include "port.h"

extern int  portInit(void);
extern int  menuInit();
extern void menuLoop();
extern int  menuAddItem(char *name, int (*cbFn)(int, char *argv[]) , char *help);
extern void* thMmeTx(void *ptr);
extern void* thMmeRx(void *ptr);
extern void* thMmeArp(void *ptr);
extern void* thS1uTx(void *ptr);

int cbS11(int argc, char *argv[] );

msgFifo_t g_msg_qs[8];
sim_port_t g_ports[4];

int main(void){	
    int i;
    pthread_t pt_mmeTx;
    pthread_t pt_mmeRx;
    pthread_t pt_mmeArp;

    pthread_t pt_s1uTx;

    portInit();

    //init msg queues
    for (i = 0; i < 6; i++) {
       initFifo(&g_msg_qs[i]);
    }
    //start stats threads
    if(pthread_create(&pt_mmeTx, NULL, &thMmeTx, NULL)) {
       printf("pthread_create failed thMmeTx \n");
       exit(1);
    }

    if(pthread_create(&pt_mmeRx, NULL, &thMmeRx, NULL)) {
       printf("pthread_create failed thMmeRx \n");
       exit(1);
    }

    if(pthread_create(&pt_mmeArp, NULL, &thMmeArp, NULL)) {
       printf("pthread_create failed thMmeArp \n");
       exit(1);
    }


    if(pthread_create(&pt_s1uTx, NULL, &thS1uTx, NULL)) {
       printf("pthread_create failed thS1uTx \n");
       exit(1);
    }
 

    menuInit();
    menuAddItem("s11", cbS11, "send s11 setup");
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

