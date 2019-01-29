#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>


#include "port.h"
#include "fifo.h"
#include "config.h"
         
extern msgFifo_t g_msg_qs[16];
extern sim_port_t g_ports[4]; 

typedef struct direction_stats_s {
    long l_packets;
    long l_bytes;
    long l_packets_old;
    long l_bytes_old;
    float     f_packet_per;
    float     f_bytes_per;
 

}direction_stats_t;

typedef struct port_stats_s {
    direction_stats_t direction[2];
}port_stats_t;

port_stats_t port_stats[4];

void* thStats(void *ptr){
    msgFifo_t *pFifo;
    int i, rtc;
    struct timespec time_old;
    struct timespec time_now;
    stats_msg_t statsMsg;
    cli_msg_t cliMsgWork;
    port_stats_t *port;
    long l_duration;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_old);
     printf("thSgiTx started\n");

    while (1){
        pFifo = &g_msg_qs[msgq_cli_to_stats];
        if((rtc = dequeueFifo(pFifo, &cliMsgWork)) != 0) {
                //printf("msg from cli %d\n",cliMsgWork.cmd);
                switch (cliMsgWork.cmd) {
                case REQ_START:
                    //print stats
                    printf("Stats:\t\t   tx pkts\t\trx pkts \t tx bytes \t rx bytes \ttxPps\trxPps\n");
                    for (i = 0; i <3; i++) {                   
                        printf("%8s  %16ld  %16ld  %16ld  %16ld  %10.4f  %10.4f\n",
                          g_ports[i].portName,
                           port_stats[i].direction[0].l_packets,
                           port_stats[i].direction[1].l_packets,
                           port_stats[i].direction[0].l_bytes,
                           port_stats[i].direction[1].l_bytes,
                           port_stats[i].direction[0].f_packet_per,
                           port_stats[i].direction[1].f_packet_per         );                    
                    }

                    break;

                default:
                    break;
                }
            }


        for (i = msgq_mmeTx_to_stats; i < msgq_sgiRx_to_stats +1; i++) {
            pFifo = &g_msg_qs[i];
            if((rtc = dequeueFifo(pFifo, &statsMsg)) != 0) {
                //printf("msg from cli %d\n",cliMsgWork.cmd);
                port = &port_stats[statsMsg.port];
                port->direction[statsMsg.direction].l_packets += (long ) statsMsg.packets;
                port->direction[statsMsg.direction].l_bytes += (long ) statsMsg.bytes;
  
            }


        }

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_now);
        l_duration =  (time_now.tv_sec - time_old.tv_sec);

         if (l_duration > ((long) 10)) { 
             time_old = time_now;
             //printf("<%ld>\n", l_duration);
             //calc rates
            for (i = 0; i < 3; i++) {
                port_stats[i].direction[0].f_packet_per = (port_stats[i].direction[0].l_packets -port_stats[i].direction[0].l_packets_old)/l_duration;
                port_stats[i].direction[0].l_packets_old = port_stats[i].direction[0].l_packets;
                
                port_stats[i].direction[1].f_packet_per = (port_stats[i].direction[1].l_packets -port_stats[i].direction[1].l_packets_old)/l_duration;
                port_stats[i].direction[1].l_packets_old = port_stats[i].direction[1].l_packets;

            }
        }
     }
}
