OBJS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard src/utility/*.cpp))
LIB = Ethernet
CPPFLAGS += -I../arduino/core -I../arduino/variants/standard -I../SPI -Isrc -DCONFIG_NODNS -DCONFIG_NOMCAST

all: lib$(LIB).a

include ../avr.mk
