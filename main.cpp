#include "config.h"
#include <Arduino.h>
#include <SdFat.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SFEMP3Shield.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <SPI.h>
#include <stdio.h>
#include "EepromBootData.h"
#include "tftp.h"
#include "http.h"
#include "mp3.h"
#include "debug.h"
#include "conffile.h"

SdFat sd;
byte resetcause __attribute__((section(".noinit")));

__attribute__((section(".init1"),naked))
void __fill_resetcause(void) {
  __asm__ __volatile__ ("sts resetcause, r2");
}

void setup() {
  wdt_reset();
  wdt_enable(WDTO_8S);
  
  DDRB |= _BV(5) | _BV(2) | _BV(0);
  PORTB |= _BV(5) | _BV(2) | _BV(0);
  _delay_ms(200);
  PORTB &= ~_BV(5);
  _delay_ms(200);
  PORTB |= _BV(5);
  _delay_ms(1000);
  PORTB &= ~_BV(5);
  
  wdt_reset();  

  EepromBootData bootdata;

#ifndef USE_DEBUG_UDP
  dbginit();
#endif
  
  if (!sd.begin(9, SPI_FULL_SPEED)) while (1); //sd.initErrorHalt();
  if (!sd.chdir(F("/"))) while (1); //sd.errorHalt_P(PSTR("sd.chdir"));

#ifndef USE_DEBUG_UDP
  dbgprintln(F("SD Card started"));
#endif

  parseConfig(bootdata);
  wdt_reset();
  
#ifndef USE_DEBUG_UDP
  char addrstr[64];

  dbgprintln(F("EEPROM data: "));
  for (int i = 0; i < 1024; i++) {
    if ((i & 15) == 0) {
      snprintf_P(addrstr, 64, PSTR("%04X: "), i);
      dbgprint(addrstr);
    }
    snprintf_P(addrstr, 64, PSTR("%02X "), eeprom_read_byte((const uint8_t *)i));
    dbgprint(addrstr);
    if ((i & 15) == 15) {
      dbgprintln();
    }
  }
  
  dbgprint(F("Hardware address: "));
  snprintf_P(addrstr, 64, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), bootdata.ifconfig.hwaddr[0], bootdata.ifconfig.hwaddr[1], bootdata.ifconfig.hwaddr[2], bootdata.ifconfig.hwaddr[3], bootdata.ifconfig.hwaddr[4], bootdata.ifconfig.hwaddr[5]);
  dbgprintln(addrstr);
  dbgprint(F("IP Address: "));
  snprintf_P(addrstr, 64, PSTR("%d.%d.%d.%d"), bootdata.ifconfig.ipaddr[0], bootdata.ifconfig.ipaddr[1], bootdata.ifconfig.ipaddr[2], bootdata.ifconfig.ipaddr[3]);
  dbgprintln(addrstr);
  dbgprint(F("Subnet: "));
  snprintf_P(addrstr, 64, PSTR("%d.%d.%d.%d"), bootdata.ifconfig.subnet[0], bootdata.ifconfig.subnet[1], bootdata.ifconfig.subnet[2], bootdata.ifconfig.subnet[3]);
  dbgprintln(addrstr);
  dbgprint(F("Gateway: "));
  snprintf_P(addrstr, 64, PSTR("%d.%d.%d.%d"), bootdata.ifconfig.gateway[0], bootdata.ifconfig.gateway[1], bootdata.ifconfig.gateway[2], bootdata.ifconfig.gateway[3]);
  dbgprintln(addrstr);
  dbgprint(F("DNS Server: "));
  snprintf_P(addrstr, 64, PSTR("%d.%d.%d.%d"), bootdata.ifconfig.dnsserver[0], bootdata.ifconfig.dnsserver[1], bootdata.ifconfig.dnsserver[2], bootdata.ifconfig.dnsserver[3]);
  dbgprintln(addrstr);
#endif

  Ethernet.begin(
    bootdata.ifconfig.hwaddr,
    bootdata.ifconfig.ipaddr,
    bootdata.ifconfig.dnsserver,
    bootdata.ifconfig.gateway,
    bootdata.ifconfig.subnet
  );
  
#ifdef DEBUG
#ifdef USE_DEBUG_UDP
  UdpDebug.begin(9);
#endif
#endif
  
  dbgbegin();
  dbgprint(F("Etherten started"));
  dbgend();

  wdt_reset();

#if DEBUG
  dbgbegin();
  dbgprint(F("Reset cause: "));
  if (resetcause & _BV(WDRF)) {
    dbgprint(F("Watchdog reset"));
  }
  if (resetcause & _BV(BORF)) {
    dbgprint(F("Brownout reset"));
  }
  if (resetcause & _BV(EXTRF)) {
    dbgprint(F("External reset"));
  }
  if (resetcause & _BV(PORF)) {
    dbgprint(F("Power-on reset"));
  }
  dbgend();
#endif

#ifdef USE_HTTP
  HTTPServer_init();
#endif

#ifdef USE_TFTP
  TFTPServer_init();
#endif
  
#ifdef USE_MP3
  MP3Player_setup();
#endif

  wdt_reset();
  wdt_enable(WDTO_1S);
}

void loop() {
  wdt_reset();
#ifdef USE_MP3
  MP3Player_loop();
#endif
#ifdef USE_HTTP
  HTTPServer_loop();
#endif
#ifdef USE_TFTP
  TFTPServer_loop();
#endif
}

int main (void) {
  setup();

  while (1) {
    loop();
  }
}
