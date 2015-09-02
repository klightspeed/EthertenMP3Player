#ifndef PORTS_H_
#define PORTS_H_

#include <avr/io.h>

#define PORT(pin) (((pin) < 8) ? PORTD : ((pin) < 14) ? PORTB : PORTC)
#define PIN(pin) (((pin) < 8) ? PIND : ((pin) < 14) ? PINB : PINC)
#define DDR(pin) (((pin) < 8) ? DDRD : ((pin) < 14) ? DDRB : DDRC)
#define PBIT(pin) (((pin) < 8) ? (pin) : ((pin) < 14) ? (pin) - 8 : (pin) - 14)
#define PORT_SET(pin) (PORT(pin) |= _BV(PBIT(pin)))
#define PORT_CLR(pin) (PORT(pin) &= ~_BV(PBIT(pin)))
#define PORT_WRITE(pin,val) do { if (val) { PORT(pin) |= _BV(PBIT(pin)); } else { PORT(pin) &= ~_BV(PBIT(pin)); } } while (0)
#define PORT_TOGGLE(pin) (PIN(pin) |= _BV(PBIT(pin)))
#define DDR_OUT(pin) (DDR(pin) |= _BV(PBIT(pin)))
#define DDR_IN(pin) (DDR(pin) &= ~_BV(PBIT(pin)))
#define PORT_MODE(pin,io) do { if (io == OUTPUT) { DDR_OUT(pin); } else { DDR_IN(pin); if (io == INPUT_PULLUP) { PORT_SET(pin); } else { PORT_CLR(pin); } } } while (0)
#define PIN_VAL(pin) (!!(PIN(pin) & _BV(PBIT(pin))))

#endif /* PORTS_H_ */
