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

#define GTP_CREATE_SESSION_REQ                               (32)
#define GTP_CREATE_SESSION_RSP                               (33)
#define GTP_MODIFY_BEARER_REQ                                (34)
#define GTP_MODIFY_BEARER_RSP                                (35)

#define GTPV2C_IFTYPE_S1U_SGW_GTPU       1
#define GTPV2C_IFTYPE_S5S8_PGW_GTPC      7
#define GTPV2C_IFTYPE_S11S4_SGW_GTPC    11

#pragma pack(1)

typedef struct gtpv2c_header_t {
	struct gtpc_t {
		uint8_t spare :3;
		uint8_t teidFlg :1;
		uint8_t piggyback :1;
		uint8_t version :3;
		uint8_t type;
		uint16_t length;
	} gtpc;
	union teid_u_t {
		struct has_teid_t {
			uint32_t teid;
			uint32_t seq :24;
			uint32_t spare :8;
		} has_teid;
		struct no_teid_t {
			uint32_t seq :24;
			uint32_t spare :8;
		} no_teid;
	} teid_u;
} gtpv2c_header;


typedef struct gtpv2c_ie_t {
	uint8_t type;
	uint16_t length;
	uint8_t instance :4;
	uint8_t spare :4;
    uint8_t data[16];
} gtpv2c_ie;

typedef struct fteid_ie_t {
	struct fteid_ie_hdr_t {
		uint8_t interface_type :6;
		uint8_t v6 :1;
		uint8_t v4 :1;
		uint32_t teid_or_gre;
	} fteid_ie_hdr;
	union ip_t {
		struct in_addr ipv4;
		struct in6_addr ipv6;
		struct ipv4v6_t {
			struct in_addr ipv4;
			struct in6_addr ipv6;
		} ipv4v6;
	} ip_u;
} fteid_ie;

typedef struct paa_ie_t {
	struct paa_ie_hdr_t {
		uint8_t pdn_type :3;
		uint8_t spare :5;
	} paa_ie_hdr;
	union ip_type_union_t {
		struct in_addr ipv4;
		struct ipv6_t {
			uint8_t prefix_length;
			struct in6_addr ipv6;
		} ipv6;
		struct paa_ipv4v6_t {
			uint8_t prefix_length;
			struct in6_addr ipv6;
			struct in_addr ipv4;
		} paa_ipv4v6;
	} ip_type_union;
} paa_ie;

#pragma pack()

int lastSeq = 0;


void decodeCreateResp(char * buff, cli_msg_t *msg){
    int i;
    int index = 12;


    gtpv2c_header *gtpHdr = (gtpv2c_header *) buff;
    gtpv2c_ie *p_ie;
    gtpv2c_ie ieDumy;
    fteid_ie  *p_fteid;
     paa_ie   *p_paa;
     simmSession_t *p_sess = (simmSession_t *)&msg->data[0];

     ieDumy.length = 0;
    if (lastSeq != gtpHdr->teid_u.has_teid.seq) {
        lastSeq = gtpHdr->teid_u.has_teid.seq;

 //       for (i = 0; i < 32; i++) {
 //           printf( "%02x ",buff[i]);
 //        }
 //       printf("\n");

        printf("  Hdr length %d type %d teid 0x%x seq %d (%d)\n",
               ntohs(gtpHdr->gtpc.length),
               gtpHdr->gtpc.type,
               ntohl(gtpHdr->teid_u.has_teid.teid),
               ntohl(gtpHdr->teid_u.has_teid.seq) >> 8,
                gtpHdr->teid_u.has_teid.seq);
        if (gtpHdr->gtpc.type == GTP_CREATE_SESSION_RSP) {
            msg->cmd = EVENT_CREATE_RESP;
        }
        else if(gtpHdr->gtpc.type == GTP_MODIFY_BEARER_RSP){
            msg->cmd = EVENT_MOD_RESP;
        }
        else {
            msg->cmd = RESP_NACK;
            return;
        }
        while (index < ntohs(gtpHdr->gtpc.length)) {
            p_ie = (gtpv2c_ie *) &buff[index];
            printf("  %d %d %d\n", index, p_ie->type, ntohs(p_ie->length));
            if (ntohs(p_ie->length) == 0 ){
                break;
            }
            switch (p_ie->type) {
            case 2:  //cause
                // if first value 16 means accepted
                if (p_ie->data[0] == 16) {
                    printf("  -accepted\n");
                }
                else {
                    printf("  -reject %d\n",p_ie->data[0]);
                }
                break;
            case 87: //IE_FTEID
                p_fteid =(fteid_ie  *) &p_ie->data[0];
                //ignore type 7 s5-s8-pgw GTPV2C_IFTYPE_S5S8_PGW_GTPC
                printf("  -fteid iftype %d, teid 0x%x, %s\n",
                       p_fteid->fteid_ie_hdr.interface_type,
                       ntohl(p_fteid->fteid_ie_hdr.teid_or_gre),
                       inet_ntoa(p_fteid->ip_u.ipv4));
                if (p_fteid->fteid_ie_hdr.interface_type == GTPV2C_IFTYPE_S1U_SGW_GTPU) {
                    p_sess->s1u_sgw_teid = ntohl(p_fteid->fteid_ie_hdr.teid_or_gre);
                    p_sess->sgw_ipAddr = p_fteid->ip_u.ipv4.s_addr;
                }

                if (p_fteid->fteid_ie_hdr.interface_type == GTPV2C_IFTYPE_S11S4_SGW_GTPC) {
                    p_sess->type11_teid = ntohl(p_fteid->fteid_ie_hdr.teid_or_gre);
                }
                break;
            case 79: //IE_PAA 
                 p_paa =(paa_ie   *)&p_ie->data[0];
                 printf("  -paa type %d %s\n", p_paa->paa_ie_hdr.pdn_type,inet_ntoa( p_paa->ip_type_union.ipv4) );
                 p_sess->ueIpAddr = p_paa->ip_type_union.ipv4.s_addr;
                break;
            case 127://IE_APN_RESTRICTION ignore
                break;

            case 73:
                    //bearer id
                    printf("  -EPS bearer id %d\n",p_ie->data[0] );
                    p_sess->epsBearerId = p_ie->data[0];
                    break;
            case 93: //IE_BEARER_CONTEXT
                //index += 4;
                p_ie = &ieDumy;

                
                break;

            default:
                printf("  -%d\n", p_ie->type);
                break;


            }

            index = index + ntohs(p_ie->length) +4;
        }


    }




}
