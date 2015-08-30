#include "config.h"
#include "SFEMP3Shield.h"
#include <avr/wdt.h>
#include "mp3.h"
#include "debug.h"

#ifdef USE_MP3
static uint32_t lastposreportmillis;
static byte MP3PlayerInitResult;
static byte MP3Dead;
SFEMP3Shield MP3Player;
char MP3_LoopFilename[16];
byte MP3_Volume = 2;

void MP3Init() {
  MP3Player.stopTrack();
  MP3Player.setVolume(MP3_Volume, MP3_Volume);
  MP3Player.setMonoMode(1);
  MP3Player.playMP3(MP3_LoopFilename);

  dbgbegin();
  dbgprint(F("Playing file "));
  dbgprint(MP3_LoopFilename);
  dbgend();

  wdt_reset();
  MP3Player.available();
  wdt_reset();

  if (MP3Player.getState() != playback) {
    dbgbegin();
    dbgprint(F("MP3 playback failed: "));
    dbgprint(MP3Player.getState());
    dbgend();

    MP3Dead = 1;
  }
}

void MP3Player_setup() {
  MP3PlayerInitResult = MP3Player.begin();

  dbgbegin();
  dbgprint(F("MP3 Player started"));
  dbgend();

  MP3Init();
}

void MP3Player_loop() {
  if (!MP3Dead) {
    MP3Player.available();

    if (MP3Player.getState() != playback) {
      dbgbegin();
      dbgprint(F("Track stopped - restarting"));
      dbgend();
      MP3Init();
    }

    if (millis() > lastposreportmillis + 5000) {
      dbgbegin();
      dbgprint(F("MP3 position: "));
      dbgprint(MP3Player.currentPosition() >> 10);
      dbgend();
      lastposreportmillis = millis();
    }
  }
}

#endif

