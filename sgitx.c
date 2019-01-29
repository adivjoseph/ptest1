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
         
extern msgFifo_t g_msg_qs[16];
extern sim_port_t g_ports[4]; 
extern simmSession_t simmSessions[100];


typedef struct sgiTxContext_s {
    uint32_t ueIp;
    uint32_t sgiIp;

} sgiTxContext_t;

#pragma pack(1)
typedef struct uePacket_s {
     ether_hdr_t    ethHdr;
     struct iphdr   ip4hdr;
     struct udphdr  udphdr;
     char           data[1500];

}uePacket_t;

#pragma pack()

extern void ph_udp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
int ph_setLengthsUe(uePacket_t *sendpkt, int pktLength);
void ph_setChecksumsUe(uePacket_t *sendpkt);
extern uint16_t ph_checksum16(void *data, unsigned int bytes);
void ph_setSeqUe(uePacket_t *sendpkt,  int ueSeq);
void pf_setUeContextUe(uePacket_t *sendpkt, sgiTxContext_t *context);

uePacket_t sendpkt;

 sgiTxContext_t sesContext[100];

void* thSgiTx(void *ptr){
    msgFifo_t *pFifo_cliIn;
    msgFifo_t *pFifo_stats;
    stats_msg_t stats_msg;
    int i, rtc;
    void * pItem = NULL;
    int sock;
    struct sockaddr_in     cpAddr; 
    int length;
    int state = 0;
    struct sockaddr_ll socket_address;
    struct ifreq ifr;
    char systemCommand[64];
    simmSession_t *p_sess = &simmSessions[0];

    sim_port_t *sp_port = &g_ports[PORT_SGI];
    cli_msg_t cliMsgWork;
    int packetsSent = 0;
    int bytesSent = 0;
    pFifo_stats = &g_msg_qs[msgq_sgiTx_to_stats];
    stats_msg.cmd = PORT_STATS;
    stats_msg.port = PORT_SGI;
    stats_msg.direction = 0;
    printf("thSgiTx started\n");
    pFifo_cliIn = &g_msg_qs[msgq_cli_to_sgiTx];
    //init here

    //build packet
    for (i = 0; i < 6; i++) {
        sendpkt.ethHdr.dest_addr[i] = sp_port->peerMac[i];
        sendpkt.ethHdr.src_addr[i] = sp_port->portMac[i];
    }
    sendpkt.ethHdr.frame_type =  htons(0x0800);
//=== ue ip
    sendpkt.ip4hdr.ihl  = 5;
    sendpkt.ip4hdr.version = 4;
    sendpkt.ip4hdr.tos = 0;
    sendpkt.ip4hdr.tot_len = 1436;   //fill in later
    sendpkt.ip4hdr.id = 1;           //fill in later
    sendpkt.ip4hdr.frag_off = 0;
    sendpkt.ip4hdr.ttl = 10;
    sendpkt.ip4hdr.protocol = 17;
    sendpkt.ip4hdr.check = 0;        //fill in later
    sendpkt.ip4hdr.saddr = htonl(sp_port->portIpAddr);
    sendpkt.ip4hdr.daddr = htonl(sp_port->peerIpAddr);
//===
    sendpkt.udphdr.source = htons(80);
    sendpkt.udphdr.dest   = htons(3000);
	sendpkt.udphdr.len = 1416;         //fill in later
	sendpkt.udphdr.check = 0;         //fill in later
//===
 

//

//  open socket
   sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock == -1) {
        perror("socket():");
        exit(1);
    }
    /*retrieve ethernet interface index*/
    strncpy(ifr.ifr_name, sp_port->portName, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    /*prepare sockaddr_ll*/
    socket_address.sll_family = PF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_IP);
    socket_address.sll_ifindex = ifr.ifr_ifindex;

    //socket_address.sll_hatype = ARPHRD_ETHER;
    //socket_address.sll_pkttype = PACKET_OTHERHOST;
   // socket_address.sll_halen = 0;
   // socket_address.sll_addr[6] = 0x00;
//socket_address.sll_addr[7] = 0x00;

    //strncpy(ifr.ifr_name, "sim_sgi", IFNAMSIZ);
    if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr,  sizeof(ifr)) < 0) {

        perror("sgi Socket bind failed");
        exit (-1);
        }
    //bind to a core
    while (1){
        if (packetsSent > 100) {
            stats_msg.packets = packetsSent;
            stats_msg.bytes = bytesSent;
            while(enqueueFifo(pFifo_stats, &stats_msg));
            packetsSent = 0;
            stats_msg.packets = 0;
            stats_msg.bytes = 0;
        }

 
       if (packetsSent == 0) {
           if((rtc = dequeueFifo(pFifo_cliIn, &cliMsgWork)) != 0) {
            //printf("msg from cli %d\n",cliMsgWork.cmd);
               switch (cliMsgWork.cmd) {
               case REQ_START:
                printf("sgiTx start cmd\n");
                for (i = 0; i <1; i++) {
                    sesContext[i].ueIp  =  ntohl(simmSessions[i].ueIpAddr);
                    sesContext[i].sgiIp =  0x0d010175;
                }

                state = 1;
                length =1200;
                pf_setUeContextUe(&sendpkt, &sesContext[0]);
                ph_setLengthsUe(&sendpkt, length);
 
                break;
               case REQ_STOP:
                    state = 0;
                    stats_msg.packets = packetsSent;
                    stats_msg.bytes = bytesSent;
                    while(enqueueFifo(pFifo_stats, &stats_msg));
                    packetsSent = 0;
                    stats_msg.packets = 0;
                    stats_msg.bytes = 0;
                    break;

               default:
                break;
               }
           }
            
        }
        if (state){

            ph_setSeqUe(&sendpkt, 1);
            // printf("1\n");
             ph_setChecksumsUe(&sendpkt);
             //length = 1450;
           //  printf("7\n");
             length = sendto(sock, (void *)&sendpkt, length, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));
             if (length <= 0) {
                 printf("send failed\n");
             }
             //printf("sgi sent %d\n", length);

            packetsSent++;
            bytesSent += length;

        }



    }

}


void pf_setUeContextUe(uePacket_t *sendpkt, sgiTxContext_t *context){

    sendpkt->ip4hdr.saddr = htonl(context->sgiIp);  
    sendpkt->ip4hdr.daddr = htonl(context->ueIp); 

    printf("sgi_setUeContext  ueIp 0x%x sgiIp 0x%x\n",  context->ueIp,context->sgiIp);

}

/**
 * 
 * 
 * @author root (1/25/19)
 * 
 * @param sendpkt 
 * @param pktLength 
 * 
 * @return int data size
 */
int ph_setLengthsUe(uePacket_t *sendpkt, int pktLength){

//1450
    sendpkt->ip4hdr.tot_len =  htons(pktLength -14);   //1436;   //fill in later

	sendpkt->udphdr.len = htons(pktLength -34);  //1416;         //fill in later



    return pktLength - 34 -8;
}


void ph_setSeqUe(uePacket_t *sendpkt,  int ueSeq){

 
    sendpkt->ip4hdr.id  = htons(ueSeq);
}


void ph_setChecksumsUe(uePacket_t *sendpkt){
    //do inner udp first
   // printf("2a\n");
    ph_udp_checksum( &sendpkt->ip4hdr, (unsigned short *) &sendpkt->udphdr);
  //  printf("3\n");
    sendpkt->ip4hdr.check = htons( ph_checksum16((void *)&sendpkt->ip4hdr, 20));
//printf("4\n");

}
