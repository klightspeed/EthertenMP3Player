#include "config.h"
#include "SdFat.h"
#include "EthernetUdp.h"

#ifdef USE_TFTP

static EthernetUDP TftpServer;
static EthernetUDP CurrentTftpClient;
static char tftpfilename[16];
static byte tftp_op;
static SdFile tftpfile;
static uint16_t CurrentTftpPort;

void TFTPReadSDFile(int blknum) {
  byte buf[64];
  int len;
  int pos = 0;
  CurrentTftpClient.beginPacket(TftpServer.remoteIP(), TftpServer.remotePort());
  buf[0] = 0;
  buf[1] = 3;
  buf[2] = (blknum >> 8) & 255;
  buf[3] = blknum & 255;
  CurrentTftpClient.write(buf, 4);
  tftpfile.seekSet((uint32_t)(blknum - 1) * 512);

  while (pos < 512) {
    int len = tftpfile.read(buf, 64);
    if (len > 0) {
      CurrentTftpClient.write(buf, len);
      pos += len;
    } else {
      break;
    }
  }

  CurrentTftpClient.endPacket();
}

void TFTPReadFlash(int blknum) {
  byte buf[64];
  int len;
  int pos = 0;
  CurrentTftpClient.beginPacket(TftpServer.remoteIP(), TftpServer.remotePort());
  buf[0] = 0;
  buf[1] = 3;
  buf[2] = (blknum >> 8) & 255;
  buf[3] = blknum & 255;
  CurrentTftpClient.write(buf, 4);

  if (blknum <= (32768 / 512)) {
    while (pos < 512) {
      memcpy_P(buf, ((const char *)((uint16_t)(blknum - 1) * 512 + pos)), 64);
      CurrentTftpClient.write(buf, 64);
      pos += 64;
    }
  }

  CurrentTftpClient.endPacket();
}

void TFTPReadFile(int blknum) {
  if (strcmp_P(tftpfilename, PSTR("flash"))) {
    TFTPReadFlash(blknum);
  } else {
    TFTPReadSDFile(blknum);
  }
}

void TFTPSendAck(unsigned int blknum) {
  byte buf[4];
  CurrentTftpClient.beginPacket(TftpServer.remoteIP(), TftpServer.remotePort());
  buf[0] = 0;
  buf[1] = 4;
  buf[2] = (blknum >> 8) & 255;
  buf[3] = blknum & 255;
  CurrentTftpClient.write(buf, 4);
  CurrentTftpClient.endPacket();
}

void TFTPWriteFile(unsigned int blknum, int len) {
  int wrlen;
  int pos = 0;
  char buf[64];

  tftpfile.seekSet((uint32_t)(blknum - 1) * 512);

  while (pos < len) {
    wrlen = len - pos;
    if (wrlen > 64) {
      wrlen = 64;
    }

    wrlen = CurrentTftpClient.read(buf, wrlen);
    tftpfile.write(buf, wrlen);
    pos += wrlen;
  }

  if (len < 512) {
    tftpfile.truncate((uint32_t)(blknum - 1) * 512 + len);
  }

  TFTPSendAck(blknum);

  if (len < 512) {
    tftpfile.sync();
    tftpfile.close();

    if (strcasecmp_P(tftpfilename, PSTR("config.txt"))) {
      asm volatile ("jmp 0x7C84");
    }
  }
}

void TFTPSendError(byte errcode, const __FlashStringHelper *errmsg) {
  byte buf[4];
  TftpServer.beginPacket(TftpServer.remoteIP(), TftpServer.remotePort());
  buf[0] = 0;
  buf[1] = 5;
  buf[2] = 0;
  buf[3] = errcode;
  TftpServer.write(buf, 4);
  TftpServer.print(errmsg);
  TftpServer.write(buf, 1);
  TftpServer.endPacket();
}

bool TFTPOpen(byte op, char *filename) {
  uint8_t mode = op == 1 ? O_READ : (O_WRITE | O_CREAT);
  return (op == 1 && !strcmp_P(filename, PSTR("flash"))) ||
         tftpfile.open(filename, mode);
}

void TFTPServer_init() {
  TftpServer.begin(69);
  CurrentTftpPort = 49152;
}

void TFTPServer_loop() {
  int pktsize = TftpServer.parsePacket();
  byte buf[64];

  if (pktsize != 0 && pktsize < 64) {
    char *filename;
    char *tftpmode;
    TftpServer.read(buf, 64);
    if (buf[0] == 0) {
      if (buf[1] == 1 || buf[1] == 2) {
        filename = (char *)(buf + 2);
        tftpmode = (char *)(buf + 3 + strlen(filename));
        filename[15] = 0;
        tftpfile.close();
        if (TFTPOpen(buf[1], filename)) {
          CurrentTftpClient.stop();
          CurrentTftpPort++;
          if (CurrentTftpPort >= 57344 || CurrentTftpPort < 49152) {
            CurrentTftpPort = 49152;
          }
          CurrentTftpClient.begin(CurrentTftpPort);
          tftp_op = buf[1];
          strncpy(tftpfilename, filename, 16);
          if (tftp_op == 1) {
            TFTPReadFile(1);
          } else {
            TFTPSendAck(0);
          }
        } else {
          if (buf[1] == 1) {
            TFTPSendError(1, F("No such file"));
          } else if (buf[1] == 2) {
            if (tftpfile.exists(filename)) {
              TFTPSendError(6, F("File exists"));
            } else {
              TFTPSendError(2, F("Bad file"));
            }
          }
        }
      } else {
        TFTPSendError(4, F("Bad request"));
      }
    }
  }

  pktsize = CurrentTftpClient.parsePacket();
  if (pktsize != 0) {
    CurrentTftpClient.read(buf, 4);
    int blknum = (int)(buf[2] << 8) | buf[3];
    if (tftp_op == 1 && buf[1] == 4) {
      TFTPReadFile(blknum + 1);
    } else if (tftp_op == 2 && buf[1] == 3) {
      TFTPWriteFile(blknum, pktsize - 4);
    }
  }
}

#endif
