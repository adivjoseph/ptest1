
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <linux/udp.h>
#include "fifo.h"
#include "port.h"
#include "config.h"

 


extern sim_port_t g_ports[4];

int ph_updateEthernetList();
int ph_findErnernetByName(char *name);
void ph_setPortIpAddr(char *cp, uint32_t *ip);
void ph_setPortMac(sim_port_t *sp_port);

char g_ethernetList[20][48];
int  g_ethernetListCount = 0;


int portInit(void){

    sim_port_t *sp_port = &g_ports[PORT_MME];
    const unsigned char ether_broadcast_addr[]=
    {0xff,0xff,0xff,0xff,0xff,0xff};
    struct sockaddr_ll addr={0};
    struct ifreq if_bind;
    int i;
    struct sockaddr_in mmeAddr;

     ph_updateEthernetList();
     printf("setting mme_s11\n");
    sp_port->portName = "mme_s11";
    sp_port->portIpStr = "10.1.10.11";
    sp_port->peerName = "cp_s11";
    sp_port->peerIpStr = "10.1.10.41";
    ph_setPortIpAddr(sp_port->portIpStr, &sp_port->portIpAddr);
    ph_setPortMac(sp_port);

    ph_setPortIpAddr(sp_port->peerIpStr, &sp_port->peerIpAddr);
    //ph_setPeerMac(sp_port);


    sp_port->sockHandle = socket(AF_INET, SOCK_DGRAM, 0);
    if (sp_port->sockHandle == -1)
    {
        printf("mme Socket creation failed\n");
        exit (-1);
    }
    // Filling mme information 
    mmeAddr.sin_family    = AF_INET; // IPv4 
    mmeAddr.sin_addr.s_addr = htonl(sp_port->portIpAddr); 
    mmeAddr.sin_port = htons(2123); 

    if ( bind(sp_port->sockHandle, (const struct sockaddr *)&mmeAddr,  
            sizeof(mmeAddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    strncpy(if_bind.ifr_name, sp_port->portName, IFNAMSIZ);
    i = ioctl(sp_port->sockHandle,  SIOCGIFINDEX, (char *)&if_bind);


    sp_port = &g_ports[PORT_S1U];
    printf("setting sim_s1u\n");
    sp_port->portName = "sim_s1u";
    sp_port->portIpStr = "11.3.1.92";
    sp_port->peerName = "dp_s1u";
    sp_port->peerIpStr = "11.3.1.93";
    ph_setPortIpAddr(sp_port->portIpStr, &sp_port->portIpAddr);
    ph_setPortMac(sp_port);
    ph_setPortIpAddr(sp_port->peerIpStr, &sp_port->peerIpAddr);


    sp_port = &g_ports[PORT_SGI];
    printf("setting sim_sgi\n");
    sp_port->portName = "sim_sgi";
    sp_port->portIpStr = "13.1.1.117";
    sp_port->peerName = "dp_sgi";
    sp_port->peerIpStr = "13.3.1.93";
    ph_setPortIpAddr(sp_port->portIpStr, &sp_port->portIpAddr);
    ph_setPortMac(sp_port);
    ph_setPortIpAddr(sp_port->peerIpStr, &sp_port->peerIpAddr);

    return 0;
}


int ph_updateEthernetList(){
    DIR *d;
    struct dirent *dir;

    d = opendir("/sys/class/net");
    g_ethernetListCount = 0;

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(strcmp(".",dir->d_name) == 0) continue;
            if(strcmp("..",dir->d_name) == 0) continue;
            //printf("%s\n", dir->d_name);
            strcpy(g_ethernetList[g_ethernetListCount], dir->d_name);
            g_ethernetListCount++;
        }
        closedir(d);
    }
    return g_ethernetListCount;
}



int ph_findErnernetByName(char *name){
    int i = 0;
    for (; i < g_ethernetListCount; i++) {
        if (strcmp(name, g_ethernetList[i]) == 0) {
            return 1;
        }
    }
    return 0;

}



void ph_setPortIpAddr(char *cpIpStr, uint32_t *ipIp){

    int i,j, k;
    char cwork[24];
    int  iwork[4];

    for (i = 0, j = 0, k = 0; i < 24; i++) {
        if (cpIpStr[i] == '.') {
            iwork[k] = atoi(cwork);
            iwork[k] = iwork[k] & 0x0ff;
            j = 0;
            k++;
        }
        else if (cpIpStr[i] == '\0') {
            iwork[k] = atoi(cwork);
            iwork[k] = iwork[k] & 0x0ff;
            if (k != 3) {
                printf("ERROR: ph_setPortIpAddr failed %s\n",cpIpStr );
            }
            break;
        }
        else {
            cwork[j] = cpIpStr[i];
            j++;
            cwork[j] = 0;
        }
    }
    *ipIp = ((0xff & iwork[0]) << 24) | ((0xff & iwork[1]) << 16) | ((0xff & iwork[2])<< 8) | (0xff & iwork[3]);
    printf("setPortIpAddr %s 0x%x\n", cpIpStr, *ipIp);

}


void ph_setPortMac(sim_port_t *sp_port){
   char systemCommand[64];
    FILE *fp,*outputfile;
    char var[40];
    int j;
    //get mac address
    sprintf(systemCommand,"cat /sys/class/net/%s/address", sp_port->portName);
    fp = popen(systemCommand, "r");
     if (fgets(var, sizeof(var), fp) != NULL) 
     {
         strcpy(sp_port->portMacStr, var);
         printf("port %s  mac %s\n",  sp_port->portName,sp_port->portMacStr );
         for (j=0; j<6; j++) {
             sp_port->portMac[j] = var[j*3 +1] < 'a' ? var[j*3 +1] - '0': var[j*3 +1] - 'a' +10;
             sp_port->portMac[j] |= var[j*3 ] < 'a' ?( var[j*3] - '0') << 4: (var[j*3] - 'a' +10) <<4;
         }
     }
     pclose(fp);
    // printf("peer %s\n",sp_port->peerName );

     sprintf(systemCommand,"cat /sys/class/net/%s/address", sp_port->peerName);
     fp = popen(systemCommand, "r");
      if (fgets(var, sizeof(var), fp) != NULL) 
      {
          strcpy(sp_port->peerMacStr, var);
          //printf("var %s\n", var);
          printf("port %s  mac %s\n",  sp_port->peerName,sp_port->peerMacStr );
          for (j=0; j<6; j++) {
              sp_port->peerMac[j] = var[j*3 +1] < 'a' ? var[j*3 +1] - '0': var[j*3 +1] - 'a' +10;
              sp_port->peerMac[j] |= var[j*3 ] < 'a' ?( var[j*3] - '0') << 4: (var[j*3] - 'a' +10) <<4;
          }
      }
      pclose(fp);


}
