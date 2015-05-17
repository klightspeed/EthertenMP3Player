#ifndef MP3_H
#define MP3_H

#include "config.h"

#ifdef USE_MP3

#include <SFEMP3Shield.h>

extern SFEMP3Shield MP3Player;
extern char MP3_LoopFilename[16];
extern byte MP3_Volume;

void MP3Player_setup();
void MP3Player_loop();

#endif

#endif
