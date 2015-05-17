#include "config.h"

#ifdef DEBUG
#ifdef USE_DEBUG_UDP
#include <EthernetUdp.h>

EthernetUDP UdpDebug;
unsigned char gbcastaddr[4] = { 255, 255, 255, 255 };

#endif
#endif

