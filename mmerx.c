#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
//#include <linux/types.h>

#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <linux/udp.h>
//#include <linux/compiler.h>
//#include <net/checksum.h>

#include "port.h"
#include "fifo.h"
#include "config.h"
         
extern msgFifo_t g_msg_qs[8];
extern sim_port_t g_ports[4]; 


char rxBuffer[2024];

void* thMmeRx(void *ptr){
    msgFifo_t *pFifo_cliIn;
    int i, rtc, len;
    void * pItem = NULL;

    struct sockaddr_in     cpAddr; 

    sim_port_t *sp_port = &g_ports[PORT_MME];

    cli_msg_t cliMsgWork;
    printf("thMmeRx started\n");
    pFifo_cliIn = &g_msg_qs[msgq_cli_to_mmeRx];
    //init here


    //bind to a core
    while (1){
  

        if((rtc = dequeueFifo(pFifo_cliIn, &cliMsgWork)) != 0) {
            //printf("msg from cli %d\n",cliMsgWork.cmd);
            switch (cliMsgWork.cmd) {
            case REQ_START:
                printf("mmeSim start cmd\n");

                break;

            default:
                break;
            }
            
        }

        //look for packets
        rtc = recvfrom(sp_port->sockHandle, (char *)rxBuffer, 2000,  
                MSG_WAITALL, ( struct sockaddr *) &cpAddr, 
                &len); 
        if (rtc) {
            printf("rx size%d\n", rtc);
        }


    }

}

