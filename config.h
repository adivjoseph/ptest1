#include <stdint.h>
typedef struct {
    int cmd;
    int arg1;
    union {
        char  data[250];
    };
} cli_msg_t;

#define STATS_SAMPLE 1000

typedef struct {
    int cmd;
    int port;
    int direction;
    int size;   // set on tx only
    int packets;
    int bytes;
    int packets_other;
} stats_msg_t;



//message queues
//cli in
#define msgq_mmeTx_to_cli   0
#define msgq_mmeRx_to_cli   1
#define msgq_stats_to_cli   2
//mmeTx in
#define msgq_cli_to_mmeTx   3
#define msgq_mmeRx_to_mmeTx 4
//mmeRx in
#define msgq_cli_to_mmeRx   5
//s1uTx in
#define msgq_cli_to_s1uTx   6
//sgiTx in
#define msgq_cli_to_sgiTx   7

//stats in
#define msgq_cli_to_stats   8
#define msgq_mmeTx_to_stats   9
#define msgq_mmeRx_to_stats   10
#define msgq_s1uTx_to_stats   11
#define msgq_s1uRx_to_stats   12
#define msgq_sgiTx_to_stats   13
#define msgq_sgiRx_to_stats   14

//message types
//cli
#define RESP_ACK    0
#define RESP_NACK   1
#define REQ_CLEAR   2
#define REQ_READ    3
#define REQ_START   4
#define REQ_STOP    5
#define REQ_STATUS  6
#define PORT_STATS  10
//mmerx
#define EVENT_CREATE_RESP   20
#define EVENT_MOD_RESP      21


typedef struct simmSession_s {
    uint16_t imsiSuffix;             //create_req value to replace at the end of imis number
    uint32_t s1u_sgw_teid;           //;  
    uint32_t sgw_ipAddr;             //         
    uint32_t s11_mme_teid;           //create_req sequentual
    uint32_t type11_teid;            //create_resp
    uint32_t ueIpAddr;               //create_resp
    uint16_t epsBearerId;            //create_resp

}simmSession_t;


#define ETH_ALEN 6
#define ETH_P_ARP 0x0806 /* Address Resolution packet */
#define ARP_HTYPE_ETHER 1  /* Ethernet ARP type */
#define ETH_P_IPv4 0x0800 /* Address Resolution packet */
#define ARP_PTYPE_IPv4 0x0800 /* Internet Protocol packet */

/* Ethernet frame header */
typedef struct {
   uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
   uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
   uint16_t frame_type;   /* Ethernet frame type */
}__attribute__((packed, aligned(1))) ether_hdr_t;



/* Ethernet ARP packet from RFC 826 */
typedef struct {
   uint16_t htype;   /* Format of hardware address */
   uint16_t ptype;   /* Format of protocol address */
   uint8_t hlen;    /* Length of hardware address */
   uint8_t plen;    /* Length of protocol address */
   uint16_t op;    /* ARP opcode (command) */
   uint8_t sha[ETH_ALEN];  /* Sender hardware address */
   uint32_t spa;   /* Sender IP address */
   uint8_t tha[ETH_ALEN];  /* Target hardware address */
   uint32_t tpa;   /* Target IP address */
}__attribute__((packed, aligned(1))) arp_ether_ipv4_t;

