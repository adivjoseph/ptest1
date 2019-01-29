#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
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
extern int lastSeq;



simmSession_t simmSessions[100];
int g_mmeSeqNumber = 1;

/* Frame (298 bytes) */
char createSessionRequestPkt[380] = {
0x48, 0x20, 0x01, 0x26, 0x00, 0x00, /* ..H .&.. */
    //6
0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 
0x01, 0x00, 0x08, 0x00, 0x10, 0x21, 0x43, 0x65, 0x87, 0x19, /* ...!Ce.. */
    //22
0x53, 0xf4,
//24 (76)msisdn
0x4c, 0x00, 0x05, 0x00,  0x04, 0x93, 0x04, 0x10, 0x02, 
//33 mei
0x4b, 0x00, 0x08, 0x00,  0x53, 0x88, 0x11, 0x50, 0x00, 0x10, 0x02, 0x00, 
//45 
0x56, 0x00, 0x0d, 0x00,  0x18, 0x04, 0xf5, 0x50, 0x00, /* ......P. */
    //54
                         0x09, 0x04, 0xf5, 0x50, 0x00, 0x00, 0x05, 0x14, /* ...P.... */
//62
0x63, 0x00, 0x01, 0x00,  0x01, 
//66 (83) serving network
0x53, 0x00, 0x03, 0x00,  0x04, 0xf5, 0x50, 
//72 (82) ran type
0x52, 0x00, 0x01, 0x00,  0x06,
//79 (77) indication
0x4d, 0x00, 0x04, 0x00,  0x00, 0x10, 0x00, 0x00,
//87 (87) f-teid
0x57, 0x00, 0x09, 0x00,  0x8a, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x01, 0x0a, 0x0b,
//110 (87)
0x57, 0x00, 0x09, 0x01,  0x87, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0x69,
//  (71)
0x47, 0x00, 0x05, 0x00, 0x04, /* ..iG.... */
0x61, 0x70, 0x6e, 0x31, 0x80, 0x00, 0x01, 0x00, /* apn1.... */
0x00, 0x4f, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, /* .O...... */
0x00, 0x00, 0x48, 0x00, 0x08, 0x00, 0x00, 0x00, /* ..H..... */
0x3e, 0x80, 0x00, 0x00, 0x3e, 0x80, 0x7f, 0x00, /* >...>... */
0x01, 0x00, 0x00, 0x4e, 0x00, 0x5e, 0x00, 0x80, /* ...N.^.. */
0xc2, 0x23, 0x23, 0x01, 0x01, 0x00, 0x23, 0x10, /* .##...#. */
0xec, 0xa3, 0x90, 0x00, 0x3e, 0xdb, 0xf9, 0x17, /* ....>... */
0xbe, 0xcf, 0xa8, 0x14, 0x8a, 0xcd, 0xde, 0x56, /* .......V */
0x55, 0x4d, 0x54, 0x53, 0x5f, 0x43, 0x48, 0x41, /* UMTS_CHA */
0x50, 0x5f, 0x53, 0x52, 0x56, 0x52, 0xc2, 0x23, /* P_SRVR.# */
0x15, 0x02, 0x01, 0x00, 0x15, 0x10, 0xb6, 0xfa, /* ........ */
0xad, 0xc5, 0x6a, 0x43, 0x6b, 0x2f, 0x0f, 0x9f, /* ..jCk/.. */
0x82, 0x35, 0x6e, 0x07, 0xd9, 0xd9, 0x80, 0x21, /* .5n....! */
0x1c, 0x01, 0x00, 0x00, 0x1c, 0x81, 0x06, 0x00, /* ........ */
0x00, 0x00, 0x00, 0x82, 0x06, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x83, 0x06, 0x00, 0x00, 0x00, 0x00, 0x84, /* ........ */
0x06, 0x00, 0x00, 0x00, 0x00, 0x72, 0x00, 0x02, /* .....r.. */
0x00, 0x0a, 0x01, 0x5f, 0x00, 0x02, 0x00, 0x72, /* ..._...r */
0x31, 0x5d, 0x00, 0x1f, 0x00, 0x49, 0x00, 0x01, /* 1]...I.. */
0x00, 0x05, 0x50, 0x00, 0x16, 0x00, 0x08, 0x09, /* ..P..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x00, 0x00                          /* .... */
};


/* Frame (47 bytes) */
char modifyBearerRequestPkt[100] = {
0x48, 0x22, 0x00, 0x2b, 0xee, 0xff, /* ..H".+.. */
0xc0, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x5d, 0x00, /* ......]. */
0x12, 0x00, 0x49, 0x00, 0x01, 0x00, 0x05, 0x57, /* ..I....W */
0x00, 0x09, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, /* ........ */
    //
0x0b, 0x01, 0x01, 0x69, 0x57, 0x00, 0x09, 0x00, /* ...iW... */
0x8a, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x01, 0x0a, /* ........ */
0x0b                                            /* . */
};

#define SESS_SEND_CREATE_REQ        0
#define SESS_WAIT_CREATE_RESP       1
#define SESS_SEND_MODIFY_REQ        2
#define SESS_WAIT_MODIFY_RESP       3

void* thMmeTx(void *ptr){
    msgFifo_t *pFifo_cliIn;
    msgFifo_t *pFifo_mmeRx;
    int i, rtc;
    void * pItem = NULL;
    struct sockaddr_in     cpAddr; 
    char systemCommand[64];
    sim_port_t *sp_port = &g_ports[PORT_MME];
    int runState = 0;       //building sessions
    int sessionIndex = 0;
    int sessionActive = 0;
    int sessState = 0;
    simmSession_t *p_sess = &simmSessions[0];

    cli_msg_t cliMsgWork;
    simmSession_t *p_sess_msg = (simmSession_t *) &cliMsgWork.data[0];
    printf("thMmeTx started\n");
    pFifo_cliIn = &g_msg_qs[msgq_cli_to_mmeTx];
    pFifo_mmeRx = &g_msg_qs[msgq_mmeRx_to_mmeTx];
    //init here

    cpAddr.sin_family    = AF_INET; // IPv4 
    cpAddr.sin_addr.s_addr = htonl(sp_port->peerIpAddr); 
    cpAddr.sin_port = htons(2123); 


    //bind to a core
    while (1){

        if (runState) {
            if (sessionIndex == sessionActive) {
                printf("mmeSim sessions %d configured\n", sessionActive);
                runState = 0;
            }
            else {

            switch (sessState) {
            case SESS_SEND_CREATE_REQ:
               p_sess->imsiSuffix = sessionIndex & 0xffff;
               p_sess->s11_mme_teid = sessionIndex;
                //modify template
                //g_mmeSeqNumber
               createSessionRequestPkt[8] = 0xff & (g_mmeSeqNumber >> 16);
               createSessionRequestPkt[9] = 0xff & (g_mmeSeqNumber >> 8);
               createSessionRequestPkt[10] = 0xff & (g_mmeSeqNumber);
               //p_sess->imsiSuffix
               createSessionRequestPkt[22] = 0xff & (sessionIndex >> 8);
               createSessionRequestPkt[23] = 0xff & (sessionIndex);
               //misdn
               createSessionRequestPkt[31] = 0xff & (sessionIndex >> 8);
               createSessionRequestPkt[32] = 0xff & (sessionIndex);

               //p_sess->s11_mme_teid
               createSessionRequestPkt[91] = 0xff & (sessionIndex >> 24);
               createSessionRequestPkt[92] = 0xff & (sessionIndex >> 16);
               createSessionRequestPkt[93] = 0xff & (sessionIndex >> 8);
               createSessionRequestPkt[94] = 0xff & (sessionIndex);
              for (i = 87; i <106; i++) {
                  printf("0x%02x ", createSessionRequestPkt[i]);
              }
              printf("\n");

               sendto(sp_port->sockHandle, (const char *)createSessionRequestPkt, 298,  
                       MSG_CONFIRM, (const struct sockaddr *) &cpAddr, 
                       sizeof(cpAddr));
               printf("creatsession sent\n");
               sessState = SESS_WAIT_CREATE_RESP;
                break;

            case SESS_WAIT_CREATE_RESP:
                if((rtc = dequeueFifo(pFifo_mmeRx, &cliMsgWork)) != 0) {
                    //printf("msg from cli %d\n",cliMsgWork.cmd);
                    if (cliMsgWork.cmd == EVENT_CREATE_RESP) {
                        printf("  <- EVENT_CREATE_RESP\n");
                        p_sess->s11_mme_teid = p_sess->s11_mme_teid;
                        p_sess->ueIpAddr = p_sess_msg->ueIpAddr;
                        p_sess->epsBearerId = p_sess_msg->epsBearerId;
                        p_sess->s1u_sgw_teid = p_sess_msg->s1u_sgw_teid;
                        p_sess->sgw_ipAddr = p_sess_msg->sgw_ipAddr;

                       g_mmeSeqNumber++;
                       sessState = SESS_SEND_MODIFY_REQ;
                    }
                }

                break;
            case SESS_SEND_MODIFY_REQ:
                printf("state: SESS_SEND_MODIFY_REQ\n");
                //modify template
                //g_mmeSeqNumber
               modifyBearerRequestPkt[8] = 0xff & (g_mmeSeqNumber >> 16);
               modifyBearerRequestPkt[9] = 0xff & (g_mmeSeqNumber >> 8);
               modifyBearerRequestPkt[10] = 0xff & (g_mmeSeqNumber);

               modifyBearerRequestPkt[30] = 0x0b;
               modifyBearerRequestPkt[31] = 0x03;
               modifyBearerRequestPkt[32] = 0x01;
               modifyBearerRequestPkt[33] = 0x5c;



                
                sendto(sp_port->sockHandle, (const char *)modifyBearerRequestPkt, 47,  
                       MSG_CONFIRM, (const struct sockaddr *) &cpAddr, 
                       sizeof(cpAddr));
               printf("modifysession sent\n");
               sessState = SESS_WAIT_MODIFY_RESP;


                break;
            case SESS_WAIT_MODIFY_RESP:
                 if((rtc = dequeueFifo(pFifo_mmeRx, &cliMsgWork)) != 0) {
                   //printf("msg from cli %d\n",cliMsgWork.cmd);
                   if (cliMsgWork.cmd == EVENT_MOD_RESP) {
                       printf("  <- EVENT_MOD_RESP\n");
                       p_sess->ueIpAddr = p_sess_msg->ueIpAddr;
                       p_sess->epsBearerId = p_sess_msg->epsBearerId;

                       printf("session %d:\n", sessionIndex);
                       printf("  s11_mme_teid 0x%0x\n", p_sess->s11_mme_teid);
                       printf("  s1u_sgw_teid 0x%0x\n", p_sess->s1u_sgw_teid);
                       printf("  sgw_ipAddr   0x%0x\n", p_sess->sgw_ipAddr);
                       printf("  ueIpAddr     0x%x\n", p_sess->ueIpAddr );
                       printf("  epsBearerId  %d\n",  p_sess->epsBearerId);
                       printf("  ueIpAddr     0x%x\n",  p_sess->ueIpAddr);
                       printf("  \n");


                      g_mmeSeqNumber++;
                      sessionIndex++;
                      sessState = SESS_SEND_CREATE_REQ;
                   }
               }
                break;
            default:
                break;
            }
            }

        }


        if((rtc = dequeueFifo(pFifo_cliIn, &cliMsgWork)) != 0) {
            //printf("msg from cli %d\n",cliMsgWork.cmd);
            switch (cliMsgWork.cmd) {
            case REQ_START:
                printf("mmeSim start cmd\n");
                if (runState) {
                    printf("mmeSim start running already\n");
                    break;
                }
                runState = 1;
                lastSeq = 0;
                sessionIndex = 0;
                sessionActive = 1;
                sessState = 0;
                p_sess = &simmSessions[0];
                //arp -i eth0 -s 192.168.0.1 00:11:22:33:44:55
                //ip neigh add 192.168.0.1 lladdr 00:11:22:33:44:55 nud permanent dev eth0
               // sprintf(systemCommand, "ip neigh add %s lladdr %s nud permanent dev %s", sp_port->portIpStr,sp_port->portMacStr, sp_port->portName);
               // sprintf(systemCommand, "arp -i %s -s %s %s", sp_port->portName, sp_port->portIpStr, sp_port->portMacStr);
               // if (system(systemCommand)) {
               //     printf("failed to set arp entry %s\n", sp_port->portIpStr);
              //  }


  
                break;
            case REQ_STOP:
                printf("mmeTx status: stop  sess created %d of %d step state %d\n",
                       sessionIndex,
                       sessionActive,
                       sessState
                       );
                runState = 0;
            case REQ_STATUS:
                printf("mmeTx status: run %d sess creat %d of %d step state %d\n", 
                       runState, 
                       sessionIndex,
                       sessionActive,
                       sessState);
                break;

            default:
                break;
            }
            
        }


    }

}
