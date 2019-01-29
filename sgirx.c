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
#include <linux/if_arp.h>
#include <linux/udp.h>
//#include <linux/compiler.h>
//#include <net/checksum.h>

#include "port.h"
#include "fifo.h"
#include "config.h"
         
extern msgFifo_t g_msg_qs[8];
extern sim_port_t g_ports[4]; 




char rcBuffer[2048];

void* thSgiRx(void *ptr){
    msgFifo_t *pFifo_cliIn;
    msgFifo_t *pFifo_stats;
    stats_msg_t stats_msg;
    int i, rtc, length;
    int sock;
    struct sockaddr_in     cpAddr; 
    sim_port_t *sp_port = &g_ports[PORT_SGI];
    arp_ether_ipv4_t *p_arpHdr = (arp_ether_ipv4_t *) &rcBuffer[14];
    ether_hdr_t *p_ethHdr = (ether_hdr_t *)rcBuffer;
    struct ifreq ifr;
    int ifindex = 0;
    struct sockaddr_ll socket_address;

	struct iphdr *ip4hdr;

    struct udphdr *udphdr;
    cli_msg_t cliMsgWork;
 
    simmSession_t *p_sess;
    int packetsSent = 0;
    int bytesSent = 0;
    pFifo_stats = &g_msg_qs[msgq_sgiRx_to_stats];
    stats_msg.cmd = PORT_STATS;
    stats_msg.port = PORT_SGI;
    stats_msg.direction = 1;



    //init here

    printf("thSgiRx started\n");
    /*open socket*/
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
    ifindex = ifr.ifr_ifindex;
    /*prepare sockaddr_ll*/
    socket_address.sll_family = PF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_IP);
    socket_address.sll_ifindex = ifindex;
//socket_address.sll_hatype = ARPHRD_ETHER;
//socket_address.sll_pkttype = PACKET_OTHERHOST;
 //   socket_address.sll_halen = 0;
 //   socket_address.sll_addr[6] = 0x00;
//socket_address.sll_addr[7] = 0x00;


    strncpy(ifr.ifr_name, "sim_sgi", IFNAMSIZ);
    if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr,  sizeof(ifr)) < 0) {

        perror("sgi Socket bind failed");
        exit (-1);
        }

//    rtc = bind(sock, (const struct sockaddr*)&socket_address, sizeof(socket_address));
//	if (rtc == -1) {
//        perror("s1uTx bind");
//exit(1);
//    }

    //bind to a core
    while (1){
        length = recvfrom(sock, rcBuffer, 1024, 0, NULL, NULL);
        if (length == -1)
        {
            perror("recvfrom():");
            exit(1);
        }
 
        //printf("sgirx ethtype 0x%x\n",htons(p_ethHdr->frame_type ));
        if(htons(p_ethHdr->frame_type) == 0x806)
        {
          //printf("sgirx arprx len %d\n", length);
            unsigned char buf_arp_dha[6];
            unsigned char buf_arp_dpa[4];

            if(htons(p_arpHdr->op) != 0x0001)
                continue;
            //is it for mme?
            //target ip address will be for mme
            if (htonl(p_arpHdr->tpa) != sp_port->portIpAddr) {
                continue;
            }

            //copy src mac address to dest
            for (i = 0; i <6; i++) {
                p_ethHdr->dest_addr[i] = p_ethHdr->src_addr[i];
            }
            // change op to resp
            p_arpHdr->op = htons(2); //response
            // copy src mac to target
            for (i = 0; i <6; i++) {
                p_arpHdr->tha[i] = p_arpHdr->sha[i];
            }
            p_arpHdr->tpa = p_arpHdr->spa;
            // fill in mme response
            for (i = 0; i <6; i++) {
                p_arpHdr->sha[i] = sp_port->portMac[i];
            }
            p_arpHdr->spa = htonl(sp_port->portIpAddr);
            //response is ready
            length = sendto(sock, rcBuffer, length, 0, (struct
                sockaddr*)&socket_address, sizeof(socket_address));
            if (length == -1)
            {
                perror("sendto():");
                exit(1);
            }
     

        }
        else{
            //printf("arp type 0x%x\n",ntohs( p_ethHdr->frame_type));
            if (p_ethHdr->dest_addr[0] == sp_port->portMac[0] &&
                p_ethHdr->dest_addr[1] == sp_port->portMac[1] &&
                p_ethHdr->dest_addr[2] == sp_port->portMac[2] &&
                p_ethHdr->dest_addr[3] == sp_port->portMac[3] &&
                p_ethHdr->dest_addr[4] == sp_port->portMac[4] &&
                p_ethHdr->dest_addr[5] == sp_port->portMac[5]) {
                ip4hdr = (struct iphdr *)&rcBuffer[14];
                udphdr = (struct udphdr *) &rcBuffer[32];
               // printf(" sgirx  match protocol %d 0x%x (0x%x) dest port %d\n",ip4hdr->protocol, ntohl(ip4hdr->daddr), sp_port->portIpAddr, ntohs(udphdr->dest) );   
                if (ip4hdr->protocol == 17 &&
                    ntohl(ip4hdr->daddr) == sp_port->portIpAddr) {
                    //for us
                    packetsSent++;
                    bytesSent += length;
                    if (packetsSent > 100) {
                        stats_msg.packets = packetsSent;
                        stats_msg.bytes = bytesSent;
                        while(enqueueFifo(pFifo_stats, &stats_msg));
                        packetsSent = 0;
                        stats_msg.packets = 0;
                        stats_msg.bytes = 0;
                    }
                    if(ntohs(udphdr->dest) == 80){
                        //printf(" sgi rsponse\n");
                       // decodeCreateResp(&rcBuffer[42], &cliMsgWork);

                      //  while(enqueueFifo(&g_msg_qs[msgq_mmeRx_to_mmeTx], &cliMsgWork));
                    }

                }
            }
        }

    }

}
