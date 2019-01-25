#include <stdint.h>
typedef struct {
    int cmd;
    int arg1;
    union {
        char  data[250];
    };
} cli_msg_t;



//message queues
//cli in
#define msgq_mmeTx_to_cli   0
#define msgq_mmeRx_to_cli   1
//mmeTx in
#define msgq_cli_to_mmeTx   2
#define msgq_mmeRx_to_mmeTx 3
//mmeRx in
#define msgq_cli_to_mmeRx   4
//s1uTx in
#define msgq_cli_to_mmeRx   4

//message types
//cli
#define RESP_ACK    0
#define RESP_NACK   1
#define REQ_CLEAR   2
#define REQ_READ    3
#define REQ_START   4
#define REQ_STOP    5
#define REQ_STATUS  6
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
