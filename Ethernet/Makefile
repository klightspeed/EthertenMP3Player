OBJS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard src/utility/*.cpp))
LIB = Ethernet
CPPFLAGS += -I../arduino/core -I../arduino/variants/standard -I../SPI -Isrc

all: lib$(LIB).a

include ../avr.mk
