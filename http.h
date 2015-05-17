#ifndef HTTP_H
#define HTTP_H

#include "config.h"

#ifdef USE_HTTP
extern char HTTP_AuthString[32];
void HTTPServer_loop();
void HTTPServer_init();
#endif

#endif
