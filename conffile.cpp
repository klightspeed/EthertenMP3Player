#include "config.h"
#include <SdFat.h>
#include <avr/pgmspace.h>
#include "EepromBootData.h"
#include "mp3.h"
#include "http.h"

void parseIP(char *val, byte *ip) {
  byte ipbyte;
  for (int i = 0; i < 4; i++) {
    ipbyte = 0;
    while (*val < '0' || *val > '9') {
      val++;
    }
    while (*val >= '0' && *val <= '9') {
      ipbyte = (ipbyte * 10) + (*val - '0');
      val++;
    }
    *(ip++) = ipbyte;
  }
}

void parseConfig(EepromBootData &bootdata) {
  SdFile cfgfile;
  if (cfgfile.open("config.txt")) {
    char key[16];
    char val[32];
    byte pos = 0;
    bool iskey = true;
    int byteread;
    key[0] = 0;
    val[0] = 0;
    do {
      byteread = cfgfile.read();
      if (iskey) {
        if (byteread == '=') {
          pos = 0;
          iskey = false;
          val[0] = 0;
        } else if (byteread == '\r' || byteread == '\n') {
          pos = 0;
        } else if (pos < 15){
          key[pos++] = (char)byteread;
          key[pos] = 0;
        }
      } else {
        if (byteread == '\r' || byteread == '\n' || byteread <= 0) {
#ifdef USE_MP3
          if (!strcasecmp_P(key, PSTR("volume"))) {
            MP3_Volume = atoi(val);
          } else if (!strcasecmp_P(key, PSTR("mp3file"))) {
            strncpy(MP3_LoopFilename, val, 16);
#endif
          } else if (!strcasecmp_P(key, PSTR("auth"))) {
            strcpy(HTTP_AuthString, val);
          } else if (!strcasecmp_P(key, PSTR("ip"))) {
            parseIP(val, bootdata.ifconfig.ipaddr);
          } else if (!strcasecmp_P(key, PSTR("gw"))) {
            parseIP(val, bootdata.ifconfig.gateway);
          } else if (!strcasecmp_P(key, PSTR("dns"))) {
            parseIP(val, bootdata.ifconfig.dnsserver);
          } else if (!strcasecmp_P(key, PSTR("netmask"))) {
            parseIP(val, bootdata.ifconfig.subnet);
          }
          iskey = true;
          key[0] = 0;
        } else if (pos < 31) {
          val[pos++] = (char)byteread;
          val[pos] = 0;
        }
      }
    } while (byteread > 0);
    
    cfgfile.close();
  }
}

