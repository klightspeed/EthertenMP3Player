SUBDIRS = SdFat arduino Ethernet
OBJS = main.o http.o debug.o conffile.o mp3.o tftp.o EepromBootData.o SFEMP3Shield/SFEMP3Shield.o SPI/SPI.o $(SUBDIRS:%=lib%.a)
PRG = mp3player

.PHONY: clean all $(SUBDIRS)

all: $(PRG).elf lst text eeprom

$(SUBDIRS):
	$(MAKE) -C $@

lib%.a: %
	cp -a $</$@ $@

%.h: %.html.gz
	python bin2h.py $@ $< $*

%.gz: %
	gzip <$< >$@

http.o: http.cpp http.h http_index.h

include avr.mk

CPPFLAGS += -ISPI -ISFEMP3Shield -Iarduino/core -Iarduino/variants/standard -IEthernet/src -ISdFat
CLEAN_FILES += $(SUBDIRS:%=lib%.a) http_index.h

