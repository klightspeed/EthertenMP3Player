OBJS = $(patsubst %.cpp,%.o,$(wildcard core/*.cpp)) $(patsubst %.c,%.o,$(wildcard core/*.c)) $(patsubst %.S,%.o,$(wildcard core/*.S))
LIB = arduino
CPPFLAGS += -Icore -Ivariants/standard

all: lib$(LIB).a

include ../avr.mk
