AVR_MK_SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

MCU = atmega328p

CC = avr-gcc
CXX = avr-g++
AS = avr-as
LD = avr-ld
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

CFLAGS += -mmcu=$(MCU) -Os -fomit-frame-pointer -Wall --std=gnu99 -ffunction-sections -fdata-sections
CXXFLAGS += -mmcu=$(MCU) -Os -fomit-frame-pointer -Wall -fno-exceptions -ffunction-sections -fdata-sections
CPPFLAGS += -DF_CPU=16000000L -DARDUINO=156 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR
ASFLAGS += -mmcu=$(MCU)
LDFLAGS += -mrelax -mmcu=$(MCU) -Wl,--gc-sections
OBJCOPYTEXT = -j .text -j .data

ifeq ($(DEBUG), yes)
CPPFLAGS += -DDEBUG
CFLAGS += -g
endif

CLEAN_FILES = $(OBJS) \
              $(PRG:%=%.elf) \
	      $(PRG:%=%.lst) \
	      $(PRG:%=%.bin) $(PRG:%=%.hex) $(PRG:%=%.srec) \
	      $(PRG:%=%_eeprom.hex) $(PRG:%=%_eeprom.bin) $(PRG:%=%_eeprom.srec) \
	      $(PRG:%=%.eps) $(PRG:%=%.pdf) $(PRG:%=%.png) \
	      $(LIB:%=lib%.a)

.PHONY: lst text eeprom hex bin srec ehex ebin esrec dox eps png pdf

$(PRG).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

lib$(LIB).a: $(OBJS)
	$(AR) crs $@ $^

clean:
	rm -f $(CLEAN_FILES)
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) $(OBJCOPYTEXT) -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) $(OBJCOPYTEXT) -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) $(OBJCOPYTEXT) -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@

# Every thing below here is used by avr-libc's build system and can be ignored
# by the casual user.

FIG2DEV                 = fig2dev
EXTRA_CLEAN_FILES       =

dox: eps png pdf

eps: $(PRG).eps
png: $(PRG).png
pdf: $(PRG).pdf

%.eps: %.fig
	$(FIG2DEV) -L eps $< $@

%.pdf: %.fig
	$(FIG2DEV) -L pdf $< $@

%.png: %.fig
	$(FIG2DEV) -L png $< $@


