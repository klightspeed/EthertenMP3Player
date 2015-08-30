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

void HTTPRespondOK(const __FlashStringHelper *contenttype) {
  CurrentHttpClient.print(F("HTTP/1.1 200 OK\nContent-Type: "));
  CurrentHttpClient.print(contenttype);
  CurrentHttpClient.print(F("\nConnection: close\n\n"));
}

void HTTPRespondOK(char *contenttype) {
  CurrentHttpClient.print(F("HTTP/1.1 200 OK\nContent-Type: "));
  CurrentHttpClient.print(contenttype);
  CurrentHttpClient.print(F("\nConnection: close\n\n"));
}

void HTTPRespondNoContent() {
  CurrentHttpClient.print(F("HTTP/1.1 204 No Content\n\n"));
  CurrentHttpClient.stop();
}

void HTTPRespondNeedAuth() {
  CurrentHttpClient.print(F("HTTP/1.1 401 Unauthorized\nWWW-Authenticate: Basic realm=\"MP3\"\n\n"));
  CurrentHttpClient.stop();
}

void HTTPRespondNotFound() {
  CurrentHttpClient.print(F("HTTP/1.1 404 Not Found\n\n"));
  CurrentHttpClient.stop();
}

void HTTPRespondBadRequest() {
  CurrentHttpClient.print(F("HTTP/1.1 400 Bad Request\n\n"));
  CurrentHttpClient.stop();
}

void HTML_WriteFileLink(char *filename, bool showdelete) {
  if (showdelete) {
    CurrentHttpClient.print(F("<form>"));
  }
  CurrentHttpClient.print(F("<a href=\""));
  CurrentHttpClient.print(filename);
  CurrentHttpClient.print(F("\">"));
  CurrentHttpClient.print(filename);
  CurrentHttpClient.print(F("</a>"));
  if (showdelete) {
    CurrentHttpClient.print(F("<input type=\"hidden\" name=\"n\" value=\""));
    CurrentHttpClient.print(filename);
    CurrentHttpClient.print(F("\"/><input type=\"button\" value=\"Delete\" onclick=\"d(this.form)\"/></form>"));
  }
}

void HTTP_ListSDFiles() {
  HTTPRespondOK(F("text/html"));
  CurrentHttpClient.print(F(
    "<!DOCTYPE html>"
    "<html>"
     "<head>"
      "<script>"
       "function s(f,m,fl){"
        "var r=new XMLHttpRequest();"
        "r.open(m,f['n'].value,0);"
        "r.send(fl);"
        "location.reload();"
       "}"
       "function p(f){"
        "s(f,'PUT',f['f'].files[0]);"
       "}"
       "function d(f){"
        "s(f,'DELETE','');"
       "}"
      "</script>"
     "</head>"
     "<body>"
#ifdef USE_MP3
      "<p>Currently playing: "
  ));
  HTML_WriteFileLink(MP3_LoopFilename, false);
  CurrentHttpClient.print(F(
      "</p>"
#endif
      "<form>"
       "<p>Upload "
        "<input type=\"file\" name=\"f\"/>"
        "as"
        "<input type=\"text\" name=\"n\"/>"
        "<input type=\"button\" value=\"Upload\" onclick=\"p(this.form)\"/>"
       "</p>"
      "</form>"
      "<ul>"
      "<li><a href=\"flash\">flash</a></li>"
  ));
  
  char filename[16];
  dir_t p;

  sd.vwd()->rewind();
  while (sd.vwd()->readDir(&p) > 0) {
    int pos = 0;
    char *ext = NULL;
    if (p.name[0] == DIR_NAME_FREE) break;
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;
    if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        ext = filename + pos;
        filename[pos++] = '.';
      }
      filename[pos++] = (char)p.name[i];
    }

    filename[pos] = 0;
    
    CurrentHttpClient.print(F("<li>"));
    HTML_WriteFileLink(filename, true);
    CurrentHttpClient.print(p.fileSize >> 10);
    CurrentHttpClient.print(F("kB</li>"));
  }
  CurrentHttpClient.print(F("</ul></body></html>"));
}

/*
void HTTP_BeginReadSDCmd(char *filename) {
  CurrentHttpFile = SdFile();
  if (CurrentHttpFile.open(filename, O_RDONLY)) {
    CurrentHttpState = HTTPState_ReadSDCmd;
    CurrentHttpPosition = 0;
  } else {
    HTTPRespondNotFound();
  }
}

void HTTP_ContinueReadSDCmd() {
  SdFile file;
  byte buf[64];
  int pos = 0;
  int len;

  while (CurrentHttpClient.connected() && (len = CurrentHttpFile.read(buf, 64)) > 0) {
    byte bufpos = 0;
    byte c;

    if (CurrentHttpPosition == 0) {
      while (bufpos < len - 1) {
        if (buf[bufpos] == '\r' || buf[bufpos] == '\n') {
          buf[bufpos] = 0;
          break;
        }

        bufpos++;
      }

      if (bufpos == len - 1) {
        buf[bufpos] = 0;
      }
      
      HTTPRespondOK((char *)buf);
      
      bufpos++;
      CurrentHttpPosition = 1;
    }

    for (int i = bufpos; i < len; i++) {
      c = buf[i];
      if (c == '@') {
        if (i > bufpos) {
          CurrentHttpClient.write(buf + bufpos, i - bufpos);
        }
        
        i++;
        
        if (i >= len) {
          c = CurrentHttpFile.read();
        } else {
          c = buf[i];
        }

        switch (c) {
          case '@': CurrentHttpClient.write('@'); break;
          case '-': CurrentHttpClient.stop(); break;
          case '!': asm volatile ("jmp 0x7C84"); break;
          case '*': asm volatile ("jmp 0x7C88"); break;
          case 't': CurrentHttpClient.print(millis() / 1000); break;
#ifdef USE_MP3
          case 'p': CurrentHttpClient.print(MP3Player.currentPosition() >> 10); break;
          case 'f': CurrentHttpClient.print(MP3_LoopFilename); break;
#endif
        }
        
        bufpos = i + 1;
      }
    }
    
    if (bufpos < len) {
      CurrentHttpClient.write(buf + pos, len - pos);
    }
    
    pos += len;
    
    if (pos >= 512) {
      return;
    }
  }

  CurrentHttpFile.close();

  if (CurrentHttpPosition == 0) {
    HTTPRespondNoContent();
  } else {
    CurrentHttpClient.stop();
  }

  CurrentHttpState = HTTPState_Init;
}
*/

bool HTTP_BeginReadSDRaw(char *filename) {
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
      CurrentHttpClient.write(buf, len);
      pos += len;

      if (pos >= 512) {
        return;
      }
    } else {
      break;
    }
  }

  CurrentHttpFile.close();
  CurrentHttpClient.stop();
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
    CurrentHttpClient.write(buf, 64);
    
    CurrentHttpPosition += 64;
    i += 64;

    if (i == 8) {
      return;
    }
  }

  CurrentHttpClient.stop();
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
    if ((len = CurrentHttpClient.read(buf, 64)) > 0) {
      CurrentHttpFile.write(buf, len);
      pos += len;
      lastread = millis();
    }
    
    if (pos >= 512 || millis() - lastread >= 10) {
      // keepalive
      CurrentHttpClient.write((byte)0);
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
  CurrentHttpClient.stop();
  asm volatile ("jmp 0x7C84");
}

void HTTP_RebootBootloader() {
  HTTPRespondNoContent();
  CurrentHttpClient.stop();
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
  
  while (CurrentHttpClient.connected() && CurrentHttpClient.available()) {
    char c = CurrentHttpClient.read();
    
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
  /*
  } else if (CurrentHttpState == HTTPState_ReadSDCmd) {
    HTTP_ContinueReadSDCmd();
  */
  } else {
    if (CurrentHttpClient.connected()) {
      CurrentHttpClient.stop();
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
            HTTP_ListSDFiles();
            return;
          } else if (!strcasecmp_P(filename, PSTR("flash"))) {
            HTTP_BeginReadFlash();
            return;
          } else if (sd.exists(filename)) {
            /*
            if (!strcasecmp_P(filename + strlen(filename) - 4, PSTR(".cmd"))) {
              HTTP_BeginReadSDCmd(filename);
              return;
            } else {
            */
              HTTP_BeginReadSDRaw(filename);
              return;
            //}
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
