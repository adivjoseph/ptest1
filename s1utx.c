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


typedef struct s1uTxContext_s {
    uint32_t teid;
    uint32_t spare;
    uint32_t ueIp;
    uint32_t sgiIp;

} s1uTxContext_t;

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

void ph_udp_checksum(struct iphdr *pIph, unsigned short *ipPayload);
int ph_setLengths(enbPacket_t *sendpkt, int pktLength);
void ph_setChecksums(enbPacket_t *sendpkt);
uint16_t ph_checksum16(void *data, unsigned int bytes);
void ph_setSeq(enbPacket_t *sendpkt, int enbSeq, int ueSeq);
void pf_setUeContext(enbPacket_t *sendpkt, s1uTxContext_t *context);
void printPkt(enbPacket_t *sendpkt, int length);



 s1uTxContext_t sesContext[100];

void* thS1uTx(void *ptr){
    msgFifo_t *pFifo_cliIn;
    msgFifo_t *pFifo_stats;
    stats_msg_t stats_msg;
    int i, rtc;
    void * pItem = NULL;
    int sock;
    struct sockaddr_in     cpAddr; 
    int length;
    struct sockaddr_ll socket_address;
    struct ifreq ifr;
    char systemCommand[64];
    simmSession_t *p_sess = &simmSessions[0];
    int seq = 0;
    enbPacket_t sendpkt;
    int packetsSent = 0;
    int bytesSent = 0;
    int state = 0;

    sim_port_t *sp_port = &g_ports[PORT_S1U];
    cli_msg_t cliMsgWork;
    printf("thS1uTx started\n");
    pFifo_cliIn = &g_msg_qs[msgq_cli_to_s1uTx];
    pFifo_stats = &g_msg_qs[msgq_s1uTx_to_stats];
    stats_msg.cmd = PORT_STATS;
    stats_msg.port = PORT_S1U;


    //init here

    //build packet
    for (i = 0; i < 6; i++) {
        sendpkt.ethHdr.dest_addr[i] = sp_port->peerMac[i];
        sendpkt.ethHdr.src_addr[i] = sp_port->portMac[i];
    }
    sendpkt.ethHdr.frame_type =  htons(0x0800);
//=== enb to sgw ip
    sendpkt.ip4hdrS1u.ihl  = 5;
    sendpkt.ip4hdrS1u.version = 4;
    sendpkt.ip4hdrS1u.tos = 0;
    sendpkt.ip4hdrS1u.tot_len = 1436;   //fill in later
    sendpkt.ip4hdrS1u.id = 1;           //fill in later
    sendpkt.ip4hdrS1u.frag_off = 0;
    sendpkt.ip4hdrS1u.ttl = 10;
    sendpkt.ip4hdrS1u.protocol = 17;
    sendpkt.ip4hdrS1u.check = 0;        //fill in later
    sendpkt.ip4hdrS1u.saddr = htonl(sp_port->portIpAddr);
    sendpkt.ip4hdrS1u.daddr = htonl(sp_port->peerIpAddr);
//===
    sendpkt.udphdrS1u.source = htons(2152);
    sendpkt.udphdrS1u.dest   = htons(2152);
	sendpkt.udphdrS1u.len = 1416;         //fill in later
	sendpkt.udphdrS1u.check = 0;         //fill in later
//===
    sendpkt.gtpu.flags = 0x30;  
    sendpkt.gtpu.type  = 0xff;   
    sendpkt.gtpu.length = 1400;           //fill in later
    sendpkt.gtpu.teid = 0;               //fill in later

//===
    sendpkt.ip4hdrUe.ihl  = 5;
    sendpkt.ip4hdrUe.version = 4;
    sendpkt.ip4hdrUe.tos = 0;
    sendpkt.ip4hdrUe.tot_len = 1400;   //fill in later
    sendpkt.ip4hdrUe.id = 1;           //fill in later
    sendpkt.ip4hdrUe.frag_off = 0;
    sendpkt.ip4hdrUe.ttl = 10;
    sendpkt.ip4hdrUe.protocol = 17;
    sendpkt.ip4hdrUe.check = 0;        //fill in later
    sendpkt.ip4hdrUe.saddr = htonl(0x123);  //fill in later
    sendpkt.ip4hdrUe.daddr = htonl(0x456);  //fill in later
//===
    sendpkt.udphdrUe.source = htons(3000);
    sendpkt.udphdrUe.dest   = htons(80);
	sendpkt.udphdrUe.len = 1380;         //fill in later
	sendpkt.udphdrUe.check = 0;         //fill in later
//data 1372
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

/*	 rtc = bind(sock, (const struct sockaddr*)&socket_address, sizeof(socket_address));
	 if (rtc == -1) {
        perror("s1uTx bind");
        exit(1);
    }
*/  strncpy(ifr.ifr_name, sp_port->portName, IFNAMSIZ);
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
                    printf("s1uTx start cmd port %s\n", sp_port->portName);
                    for (i = 0; i <1; i++) {
                        sesContext[i].teid  =  simmSessions[i].s1u_sgw_teid;
                        sesContext[i].ueIp  =  simmSessions[i].ueIpAddr;
                        sesContext[i].sgiIp =  0x0d010175;
                    }
                    length =1200;
                    state = 1;
                    pf_setUeContext(&sendpkt, &sesContext[0]);
                    ph_setLengths(&sendpkt, length);
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

            ph_setSeq(&sendpkt, seq, seq);
            seq++;
           // printf("1\n");
            ph_setChecksums(&sendpkt);
            //length = 1450;
          //  printf("7\n");
            //printPkt(&sendpkt, length);
            length = sendto(sock, (void *)&sendpkt, length, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));
            if (length <= 0) {
                printf("send failed\n");
            }
//            printf("siu sent %d\n", length);
            packetsSent++;
            bytesSent += length;

        }


    }

}


void printPkt(enbPacket_t *sendpkt, int length){

    uint8_t *pkt = (uint8_t *) sendpkt;
    int i = 0;

    for (i = 0; i < length; i++) {
        printf(" %02x", pkt[i]);
        if (i != 0 && i%16 == 0) {
            printf("\n");
        }
    }
    printf("\n");

}


void pf_setUeContext(enbPacket_t *sendpkt, s1uTxContext_t *context){

    sendpkt->gtpu.teid = htonl(context->teid);
    sendpkt->ip4hdrUe.saddr = htonl(context->ueIp);  
    sendpkt->ip4hdrUe.daddr = htonl(context->sgiIp); 

    printf("pf_setUeContext teid 0x%0x ueIp 0x%x sgiIp 0x%x\n", context->teid, context->ueIp,context->sgiIp);

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
int ph_setLengths(enbPacket_t *sendpkt, int pktLength){

//1450
    sendpkt->ip4hdrS1u.tot_len =  htons(pktLength -14);   //1436;   //fill in later

	sendpkt->udphdrS1u.len = htons(pktLength -34);  //1416;         //fill in later

    sendpkt->gtpu.length = htons(pktLength -50);  //1400;           //fill in later

    sendpkt->ip4hdrUe.tot_len = htons(pktLength -50);  //1400;   //fill in later

	sendpkt->udphdrUe.len = htons(pktLength -70);   //1380;         //fill in later
    //data 1372
    return pktLength - 78;
}


void ph_setSeq(enbPacket_t *sendpkt, int enbSeq, int ueSeq){

    sendpkt->ip4hdrS1u.id = htons(enbSeq);
    sendpkt->ip4hdrUe.id  = htons(ueSeq);
}


void ph_setChecksums(enbPacket_t *sendpkt){
    //do inner udp first
   // printf("2a\n");
    ph_udp_checksum( &sendpkt->ip4hdrUe, (unsigned short *) &sendpkt->udphdrUe);
  //  printf("3\n");
    sendpkt->ip4hdrUe.check = htons( ph_checksum16((void *)&sendpkt->ip4hdrUe, 20));
//printf("4\n");
    //outer udp
    ph_udp_checksum( &sendpkt->ip4hdrS1u, (unsigned short *) &sendpkt->udphdrS1u);
  //      printf("5\n");
    sendpkt->ip4hdrS1u.check = htons( ph_checksum16((void *)&sendpkt->ip4hdrS1u, 20));
  //  printf("6\n");
}

uint16_t ph_checksum16(void *data, unsigned int bytes){
    uint16_t *data_pointer = (uint16_t *) data;
    uint32_t total_sum;

    while(bytes > 1){
        total_sum += ntohs(*data_pointer++);
        //If it overflows to the MSBs add it straight away
        if(total_sum >> 16){
            total_sum = (total_sum >> 16) + (total_sum & 0x0000FFFF);
        }
        bytes -= 2; //Consumed 2 bytes
    }
    if(1 == bytes){
        //Add the last byte
        total_sum += ntohs(*(((uint8_t *) data_pointer) + 1));
        //If it overflows to the MSBs add it straight away
        if(total_sum >> 16){
            total_sum = (total_sum >> 16) + (total_sum & 0x0000FFFF);
        }
        bytes -= 1;
    }

    return (~((uint16_t) total_sum));
}



void ph_udp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {

    register unsigned long sum = 0;

    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);

    struct udphdr *udphdrp = (struct udphdr*)(ipPayload);

    //add the pseudo header 

    //the source ip

    sum += (pIph->saddr>>16)&0xFFFF;

    sum += (pIph->saddr)&0xFFFF;

    //the dest ip

    sum += (pIph->daddr>>16)&0xFFFF;

    sum += (pIph->daddr)&0xFFFF;

    //protocol and reserved: 6

    sum += htons(IPPROTO_UDP);

    //the length

    sum += htons(tcpLen);

 

    //add the IP payload

    //initialize checksum to 0

    udphdrp->check = 0;

    while (tcpLen > 1) {

        sum += * ipPayload++;

        tcpLen -= 2;

    }

    //if any bytes left, pad the bytes and add

    if(tcpLen > 0) {

        //printf("+++++++++++padding, %dn", tcpLen);

        sum += ((*ipPayload)&htons(0xFF00));

    }

      //Fold 32-bit sum to 16 bits: add carrier to result

      while (sum>>16) {

          sum = (sum & 0xffff) + (sum >> 16);

      }

      sum = ~sum;

    //set computation result

    udphdrp->check = (unsigned short)sum;

}
