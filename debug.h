#ifndef DEBUG_H
#define DEBUG_H

#include "config.h"

#ifdef DEBUG
#ifdef USE_DEBUG_UDP
#include <EthernetUdp.h>
extern EthernetUDP UdpDebug;
extern unsigned char gbcastaddr[4];
#define dbginit() UdpDebug.begin(9)
#define dbgbegin() UdpDebug.beginPacket(gbcastaddr, 9)
#define dbgprint(...) UdpDebug.print(__VA_ARGS__)
#define dbgprintln(...) UdpDebug.println(__VA_ARGS__)
#define dbgend() UdpDebug.endPacket()
#else
#define dbginit() Serial.begin(115200)
#define dbgbegin() void(0)
#define dbgprint(...) Serial.print(__VA_ARGS__)
#define dbgprintln(...) Serial.println(__VA_ARGS__)
#define dbgend() Serial.println()
#endif
#else
#define dbginit() void(0)
#define dbgbegin() void(0)
#define dbgprint(...) void(0)
#define dbgprintln(...) void(0)
#define dbgend() void(0)
#endif


#endif
