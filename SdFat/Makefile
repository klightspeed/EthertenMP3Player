OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIB = SdFat
CPPFLAGS += -I../arduino/core -I../arduino/variants/standard -I../SPI -I. -Iutility

all: lib$(LIB).a

include ../avr.mk
