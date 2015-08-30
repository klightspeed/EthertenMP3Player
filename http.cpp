#include "config.h"
#include "SdFat.h"
#include "Ethernet.h"
#include "http.h"
#include "mp3.h"
#include "debug.h"

#ifdef USE_HTTP

enum HTTPState {
    HTTPState_Init,
    HTTPState_WriteSD,
    HTTPState_ReadFlash,
    HTTPState_ReadSDRaw,
    HTTPState_ReadSDCmd
};

static EthernetServer HttpServer(80);
static EthernetClient CurrentHttpClient;
static SdFile CurrentHttpFile;
static enum HTTPState CurrentHttpState;
static uint32_t CurrentHttpPosition;
char HTTP_AuthString[32];
static char recvbuf[128];
static byte recvbuflen;
static byte recvbufpos;

static inline size_t client_write(const __FlashStringHelper *data, size_t len) {
  if (len > 64) {
    char _data[64];
    int ret = 0;
    while (ret < len) {
      int _len = len - ret;
      int _retlen;

      if (_len > 64) {
        _len = 64;
      }

      memcpy_P(_data, ((PGM_P)data) + ret, 64);
      _retlen = CurrentHttpClient.write(_data, _len);
      if (_retlen == 0) {
        break;
      }
      ret += _retlen;
    }
    return ret;
  } else {
    char _data[len];
    memcpy_P(_data, (PGM_P)data, len);
    return CurrentHttpClient.write(_data, len);
  }
}

static inline size_t client_write(const char *data, size_t len) {
  return CurrentHttpClient.write(data, len);
}

static inline size_t client_write(const __FlashStringHelper *data) {
  return client_write(data, strlen_P((PGM_P)data));
}

static inline size_t client_write(const char *data) {
  return client_write(data, strlen(data));
}

static inline size_t client_write(int val) {
  char _data[2 + 3 * sizeof(int)];
  itoa(val, _data, 10);
  return client_write(_data, strlen(_data));
}

static inline size_t client_write(unsigned int val) {
  char _data[1 + 3 * sizeof(unsigned int)];
  utoa(val, _data, 10);
  return client_write(_data, strlen(_data));
}

static size_t client_read(char *data, size_t len) {
  int pos = 0;

  if (recvbufpos < recvbuflen) {
    pos = len;

    if (pos > (recvbuflen - recvbufpos)) {
      pos = recvbuflen - recvbufpos;
    }

    memcpy(data, recvbuf + recvbufpos, pos);

    recvbufpos += pos;
  }

  if (pos == len) {
    return len;
  }

  if (len - pos >= 32) {
    return pos + CurrentHttpClient.read((uint8_t *)data + pos, len - pos);
  } else {
    recvbuflen = CurrentHttpClient.read((uint8_t *)recvbuf, sizeof(recvbuf));
    recvbufpos = len - pos;

    if (recvbufpos > recvbuflen) {
      recvbufpos = recvbuflen;
    }

    memcpy(data + pos, recvbuf, recvbufpos);

    return pos + recvbufpos;
  }
}

static inline char client_read() {
  char c;
  client_read(&c, 1);
  return c;
}

static inline bool client_avail() {
  return recvbufpos < recvbuflen || (CurrentHttpClient.connected() && CurrentHttpClient.available());
}

static inline void client_close() {
  CurrentHttpClient.stop();
  recvbufpos = recvbuflen = 0;
}

void HTTPRespondOK(const __FlashStringHelper *contenttype) {
  client_write(F("HTTP/1.1 200 OK\nContent-Type: "));
  client_write(contenttype);
  client_write(F("\nConnection: close\n\n"));
}

void HTTPRespondOK(char *contenttype) {
  client_write(F("HTTP/1.1 200 OK\nContent-Type: "));
  client_write(contenttype);
  client_write(F("\nConnection: close\n\n"));
}

void HTTPRespondNoContent() {
  client_write(F("HTTP/1.1 204 No Content\n\n"));
  client_close();
}

void HTTPRespondNeedAuth() {
  client_write(F("HTTP/1.1 401 Unauthorized\nWWW-Authenticate: Basic realm=\"MP3\"\n\n"));
  client_close();
}

void HTTPRespondNotFound() {
  client_write(F("HTTP/1.1 404 Not Found\n\n"));
  client_close();
}

void HTTPRespondBadRequest() {
  client_write(F("HTTP/1.1 400 Bad Request\n\n"));
  client_close();
}

void HTTP_ListSDFilesJson() {
  char buf[64];
  uint8_t bufpos = 0;
  bool first = true;
  HTTPRespondOK(F("application/json"));
#ifdef USE_MP3
  strcpy_P(buf, PSTR("{\"mp3\":\""));
  strcat(buf, MP3_LoopFilename);
  strcat_P(buf, PSTR("\",\"pos\":\""));
  ltoa(MP3Player.currentPosition(), buf + strlen(buf), 10);
  strcat_P(buf, PSTR("\",\"files\":["));
#else
  strcpy_P(buf, PSTR("{\"files\":["));
#endif
  client_write(buf);

  char filename[16];
  dir_t p;

  sd.vwd()->rewind();
  while (sd.vwd()->readDir(&p) > 0) {
    int pos = 0;
    char *ext = NULL;
    if (p.name[0] == DIR_NAME_FREE) break;
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;
    if (!DIR_IS_FILE(&p)) continue;

    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        filename[pos++] = '.';
      }
      filename[pos++] = (char)p.name[i];
    }

    filename[pos] = 0;

    buf[0] = 0;

    if (!first) {
      buf[0] = ',';
      buf[1] = 0;
    }

    first = false;

    strcat_P(buf, PSTR("{\"name\":\""));
    strcat(buf, filename);
    strcat_P(buf, PSTR("\",\"size\":"));
    ltoa(p.fileSize, buf + strlen(buf), 10);
    strcat_P(buf, PSTR("}"));

    client_write(buf);
  }

  buf[0] = ']';
  buf[1] = '}';
  buf[2] = 0;

  client_write(buf, 2);
}

void HTTP_PrintIndex() {
  HTTPRespondOK(F("text/html"));
  client_write(F("<!DOCTYPE html>\n<html>\n<body>\n<script type=\"text/javascript\">\nfunction s(f,m,fl){\nvar r=new XMLHttpRequest();\nr.open(m,f['n'].value,0);\nr.send(fl);\nlocation.reload();\n}\nfunction p(f){\ns(f,'PUT',f['f'].files[0]);\n}\nfunction d(f){\ns(f,'DELETE','');\n}\nfunction l(){\nvar r=new XMLHttpRequest();\nr.open(\"GET\",\"/files\",0);\nr.send();\nvar fi=JSON.parse(r.responseText);\nvar w=function(c){document.write(c);};\nif('mp3' in fi){\nw('<p>Currently playing: <a href=\"'+fi.mp3+'\">'+fi.mp3+'</a></p><p>Current position: '+(fi.pos/1024)+'s</p>');\n}\nw('<form><p>Upload:</p><p>File:<input type=\"file\" name=\"f\"/></p><p>Name:<input type=\"text\" name=\"n\"/></p><p><input type=\"button\" value=\"Upload\" onclick=\"p(this.form)\"/></p></form>');\nw('<table><tr><th>Name</th><th>Size</th><th>&nbsp;</th></tr><tr><td><a href=\"flash\">flash</a></td><td>32768</td><td>&nbsp;</td></tr>');\nfi.files.forEach(function(f){\nw('<tr><td><a href=\"'+f.name+'\">'+f.name+'</a></td><td>'+f.size+'</td><td><form><input type=\"hidden\" name=\"n\" value=\"'+f.name+'\"/><input type=\"button\" value=\"Delete\" onclick=\"d(this.form)\"/></form></td></tr>');\n});\nw('</table>');\n}\nl();\n</script>\n</body>\n</html>"));
}

void HTTP_BeginReadSDRaw(char *filename) {
  CurrentHttpFile = SdFile();
  if (CurrentHttpFile.open(filename, O_RDONLY)) {
    HTTPRespondOK(F("application/octet-stream"));
    CurrentHttpState = HTTPState_ReadSDRaw;
  } else {
    HTTPRespondNotFound();
  }
}

void HTTP_ContinueReadSDRaw() {
  byte buf[64];
  int len;
  int pos = 0;

  while (CurrentHttpClient.connected()) {
    len = CurrentHttpFile.read(buf, 64);

    if (len > 0) {
      client_write((const char *)buf, len);
      pos += len;

      if (pos >= 512) {
        return;
      }
    } else {
      break;
    }
  }

  CurrentHttpFile.close();
  client_close();
  CurrentHttpState = HTTPState_Init;
}

void HTTP_BeginReadFlash() {
  HTTPRespondOK(F("application/octet-stream"));
  CurrentHttpState = HTTPState_ReadFlash;
  CurrentHttpPosition = 0;
}

void HTTP_ContinueReadFlash() {
  uint16_t i = 0;
  byte buf[64];

  while (CurrentHttpPosition < 32768 && CurrentHttpClient.connected()) {
    memcpy_P(buf, (const void *)CurrentHttpPosition, 64);
    client_write((const char *)buf, 64);

    CurrentHttpPosition += 64;
    i += 64;

    if (i == 8) {
      return;
    }
  }

  client_close();
  CurrentHttpState = HTTPState_Init;
}

void HTTP_BeginWriteSD(char *filename) {
  CurrentHttpFile = SdFile();
  if (CurrentHttpFile.open(filename, O_WRONLY | O_CREAT | O_TRUNC)) {
    CurrentHttpState = HTTPState_WriteSD;
  } else {
    HTTPRespondBadRequest();
  }
}

void HTTP_ContinueWriteSD() {
  byte buf[64];
  int len;
  int pos = 0;
  uint32_t lastread = millis();

  while (CurrentHttpClient.connected()) {
    if ((len = client_read((char *)buf, 64)) > 0) {
      CurrentHttpFile.write(buf, len);
      pos += len;
      lastread = millis();
    }

    if (pos >= 512 || millis() - lastread >= 10) {
      // keepalive
      client_write("", 1);
      return;
    }

    if (millis() - lastread > 1024) {
      break;
    }
  }

  CurrentHttpFile.sync();
  CurrentHttpFile.close();
  HTTPRespondNoContent();

  CurrentHttpState = HTTPState_Init;
}


void HTTP_DeleteSD(char *filename) {
  SdFile file;

  if (file.open(filename, O_WRONLY)) {
    file.remove();
    HTTPRespondNoContent();
  } else {
    HTTPRespondBadRequest();
  }
}

void HTTP_RebootApplication() {
  HTTPRespondNoContent();
  client_close();
  asm volatile ("jmp 0x7C84");
}

void HTTP_RebootBootloader() {
  HTTPRespondNoContent();
  client_close();
  asm volatile ("jmp 0x7C88");
}

bool HTTPParseRequest(char *method, char *filename, int maxnamelen) {
  char key[16];
  char val[32];
  int clindex = 0;
  int column = 0;
  int tokindex = 0;
  bool iskey = false;
  char lastc = 0;
  bool authenticated = false;
  filename[0] = 0;
  method[0] = 0;
  val[0] = 0;

  while (client_avail()) {
    char c = client_read();

    if (c == '\n') {
      if (column == 0) {
        break;
      }
      if (tokindex >= 2) {
        if (!strcasecmp_P(key, PSTR("Authorization"))) {
          if (!strcmp(val, HTTP_AuthString)) {
            authenticated = true;
          }
        }
      }
      tokindex = 2;
      column = 0;
      clindex = 0;
      iskey = true;
      continue;
    } else if (c == '\r') {
      continue;
    } else if (c == ' ' || c == ':') {
      c = 0;
      if (key[0] != 0) {
        iskey = false;
      }
    }

    if (c != 0) {
      if (tokindex == 0 && clindex < 7) {
        method[clindex++] = c;
        method[clindex] = 0;
      } else if (tokindex == 1 && clindex < maxnamelen - 1 && (clindex != 0 || c != '/')) {
        filename[clindex++] = c;
        filename[clindex] = 0;
      } else if (tokindex >= 2 && iskey && clindex < 15) {
        key[clindex++] = c;
        key[clindex] = 0;
      } else if (tokindex >= 2 && !iskey && clindex < 31) {
        val[clindex++] = c;
        val[clindex] = 0;
      }
    } else if (lastc != 0) {
      tokindex++;
      clindex = 0;
    }

    lastc = c;
    column++;
  }

  if (!authenticated && HTTP_AuthString[0] != 0) {
    HTTPRespondNeedAuth();
  }

  return authenticated || HTTP_AuthString[0] == 0;
}

void HTTPServer_loop() {
  if (CurrentHttpState == HTTPState_WriteSD) {
    HTTP_ContinueWriteSD();
  } else if (CurrentHttpState == HTTPState_ReadFlash) {
    HTTP_ContinueReadFlash();
  } else if (CurrentHttpState == HTTPState_ReadSDRaw) {
    HTTP_ContinueReadSDRaw();
  } else {
    if (CurrentHttpClient.connected()) {
      client_close();
    }

    CurrentHttpClient = HttpServer.available();

    if (CurrentHttpClient.connected()) {
      dbgbegin();
      dbgprint(F("Client connected"));
      dbgend();
      char method[8];
      char filename[32];

      if (HTTPParseRequest(method, filename, 32)) {
        dbgbegin();
        dbgprint(F("Method: "));
        dbgprint(method);
        dbgprint(F("; filename: "));
        dbgprint(filename);
        dbgend();
        if (!strcasecmp_P(method, PSTR("GET"))) {
          CurrentHttpClient.flush();
          if (filename[0] == 0) {
            HTTP_PrintIndex();
            return;
          } else if (!strcasecmp_P(filename, PSTR("flash"))) {
            HTTP_BeginReadFlash();
            return;
	  } else if (!strcasecmp_P(filename, PSTR("files"))) {
	    HTTP_ListSDFilesJson();
	    return;
          } else if (sd.exists(filename)) {
            HTTP_BeginReadSDRaw(filename);
            return;
          }
        } else if (!strcasecmp_P(method, PSTR("POST"))) {
          if (strstr_P(filename, PSTR("delete/")) == filename) {
            HTTP_DeleteSD(filename + 7);
            return;
          } else if (!strcmp_P(filename, PSTR("reboot"))) {
            HTTP_RebootApplication();
            __builtin_unreachable();
          } else if (!strcmp_P(filename, PSTR("bootloader"))) {
            HTTP_RebootBootloader();
            __builtin_unreachable();
          }
        } else if (!strcasecmp_P(method, PSTR("PUT"))) {
          HTTP_BeginWriteSD(filename);
          return;
        } else if (!strcasecmp_P(method, PSTR("DELETE"))) {
          HTTP_DeleteSD(filename);
          return;
        }

        HTTPRespondNotFound();
      }
    }
  }
}

void HTTPServer_init() {
  HttpServer.begin();

  dbgbegin();
  dbgprint(F("HTTP Server started"));
  dbgend();
}

#endif
