#ifndef TFTP_H
#define TFTP_H

#include "config.h"

#ifdef USE_TFTP

void TFTPServer_init();
void TFTPServer_loop();

#endif

#endif
