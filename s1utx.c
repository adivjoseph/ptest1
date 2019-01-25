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
extern simmSession_t simmSessions[100];
#pragma pack(1)

typedef struct gtpv1_header_t {
	uint8_t flags;
	uint8_t type;
	uint16_t length;
	uint32_t teid;
} gtpv1_header;


typedef struct enbPacket_s {
     ether_hdr_t    ethHdr;
     struct iphdr   ip4hdrS1u;
     struct udphdr  udphdrS1u;
     gtpv1_header   gtpu;
     struct iphdr   ip4hdrUe;
     struct udphdr  udphdrUe;
     char           data[1500];

}enbPacket_t;

#pragma pack()

enbPacket_t sendpkt;

void* thS1uTx(void *ptr){
    msgFifo_t *pFifo_cliIn;
    int i, rtc;
    void * pItem = NULL;
    struct sockaddr_in     cpAddr; 
    char systemCommand[64];
    simmSession_t *p_sess = &simmSessions[0];

    sim_port_t *sp_port = &g_ports[PORT_S1U];
    cli_msg_t cliMsgWork;
    printf("thS1uTx started\n");
    pFifo_cliIn = &g_msg_qs[msgq_cli_to_s1uTx];
    //init here

    //build packet
    for (i = 0; i < 6; i++) {
        sendpkt.ethHdr.dest_addr[i] = sp_port->peerMac[i];
        sendpkt.ethHdr.src_addr[i] = sp_port->portMac[i];
    }
    sendpkt.ethHdr.frame_type =  htons(0x0800);
//===
    sendpkt.ip4hdrS1u.ihl  = 5;
    sendpkt.ip4hdrS1u.version = 4;
    sendpkt.ip4hdrS1u.tos = 0;
    sendpkt.ip4hdrS1u.tot_len = 1000;   //fill in later
    sendpkt.ip4hdrS1u.id = 1;           //fill in later
    sendpkt.ip4hdrS1u.frag_off = 0;
    sendpkt.ip4hdrS1u.ttl = 10;
    sendpkt.ip4hdrS1u.protocol = 17;
    sendpkt.ip4hdrS1u.check = 0;        //fill in later
    sendpkt.ip4hdrS1u.saddr = hotnl(sp_port->portIpAddr);
    sendpkt.ip4hdrS1u.daddr = hotnl(sp_port->peerIpAddr);
//===
    sendpkt.udphdrS1u.source = htons(2152);
    sendpkt.udphdrS1u.dest   = htons(2152);
	sendpkt.udphdrS1u.len = 100;         //fill in later
	sendpkt.udphdrS1u.check = 0;         //fill in later
//===
    senpkt.gtpu.flags = 0x30;  
    senpkt.gtpu.type  = 0xff;   
    senpkt.gtpu.length = 100;           //fill in later
    senpkt.gtpu.teid = 0;               //fill in later

//===
    sendpkt.ip4hdrUe.ihl  = 5;
    sendpkt.ip4hdrUe.version = 4;
    sendpkt.ip4hdrUe.tos = 0;
    sendpkt.ip4hdrUe.tot_len = 1000;   //fill in later
    sendpkt.ip4hdrUe.id = 1;           //fill in later
    sendpkt.ip4hdrUe.frag_off = 0;
    sendpkt.ip4hdrUe.ttl = 10;
    sendpkt.ip4hdrUe.protocol = 17;
    sendpkt.ip4hdrUe.check = 0;        //fill in later
    sendpkt.ip4hdrUe.saddr = hotnl(0x123);  //fill in later
    sendpkt.ip4hdrUe.daddr = hotnl(0x456);  //fill in later
//===
    sendpkt.udphdrUe.source = htons(3000);
    sendpkt.udphdrUe.dest   = htons(80);
	sendpkt.udphdrUe.len = 100;         //fill in later
	sendpkt.udphdrUe.check = 0;         //fill in later




    //bind to a core
    while (1){

 

        if((rtc = dequeueFifo(pFifo_cliIn, &cliMsgWork)) != 0) {
            //printf("msg from cli %d\n",cliMsgWork.cmd);
            switch (cliMsgWork.cmd) {
            case REQ_START:
                printf("s1uTx start cmd\n");
 
                break;


            default:
                break;
            }
            
        }


    }

}
