#include <stdint.h>

typedef struct sim_port_s {
    char *portName;  
    char portMac[6];
    char portMacStr[24];
    char *portIpStr;
    uint32_t portIpAddr;
    int sockHandle;
//peer
    char *peerName;  
    char peerMac[6];
    char peerMacStr[24];
    char *peerIpStr;
    uint32_t peerIpAddr;
}sim_port_t;

#define PORT_MME    0
#define PORT_S1U    1
#define PORT_SGI    2


