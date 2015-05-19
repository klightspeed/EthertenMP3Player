#ifndef EEPROM_BOOT_DATA
#define EEPROM_BOOT_DATA

#include <stdint.h>

struct EepromBootData {
    uint8_t sig[2];
    uint8_t structlen;
    struct {
        uint8_t gateway[4];
        uint8_t subnet[4];
        uint8_t hwaddr[6];
        uint8_t ipaddr[4];
	uint8_t dhcpserver[4];
	char hostname[16];
	uint8_t dnsserver[4];
	uint8_t bcastaddr[4];
    } ifconfig;
    uint8_t usedhcp;
    struct {
        unsigned long epoch;
        unsigned long sec;
    } startup_time;
    char firmware_filename[64];

    EepromBootData();

    void update();
};

#endif
