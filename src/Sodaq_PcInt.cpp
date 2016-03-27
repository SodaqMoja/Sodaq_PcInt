/*
 * Sodaq_PcInt.cpp
 *
 * Copyright (c) 2014 Kees Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * This module supplies a set of helper functions to use the
 * PinChange interrupt in a convenient manner, similar to
 * the standard Arduino attachInterrupt.  It was created with
 * inspiration from PcInt, PinChangeInt and PciManager.  The main
 * goal was to keep it simple and small as possible.
 *
 * The handler prototype is void (*)(void).  This makes is identical
 * to the call backs for Ardiuno's attachInterrupt.
 *
 * A simple example of its usage is as follows:
 *
 *   #include <PcInt.h>
 *
 *   void setup()
 *   {
 *     pinMode(A0, INPUT_PULLUP);
 *     PcInt::attachInterrupt(A0, handleA0);
 *   }
 *
 *   void handlerA0()
 *   {
 *     // pin A0 changed, do something
 *   }
 *
 * The user program is responsible to look at the I/O pin and see what
 * happened.  The original PcInt keeps track of old port values so that
 * it can see which of the port pins changed.
 */

#include "Sodaq_PcInt.h"
#include "PinChangeInterruptBoards.h"
#include <avr/interrupt.h>

class PcIntPort
{
public:
  PcInt::callback_arg funcs[8];
  void* args[8];
  uint8_t state;
  uint8_t rising;
  uint8_t falling;
  
  PcIntPort() 
  : funcs({nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}),
    state(0),
    rising(0),
    falling(0)
  { }
};


#if defined(PCINT_INPUT_PORT0)
PcIntPort port0;
#endif
#if defined(PCINT_INPUT_PORT1)
PcIntPort port1;
#endif
#if defined(PCINT_INPUT_PORT2)
PcIntPort port2;
#endif
#if defined(PCINT_INPUT_PORT3)
PcIntPort port3;
#endif


static inline PcIntPort* get_port(uint8_t port) {
    switch (port) {
#if defined(PCINT_INPUT_PORT0)
        case 0: return &port0;
#endif
#if defined(PCINT_INPUT_PORT1)
        case 1: return &port1;
#endif
#if defined(PCINT_INPUT_PORT2)
        case 2: return &port2;
#endif
#if defined(PCINT_INPUT_PORT3)
        case 3: return &port3;
#endif
        default: return nullptr;
    }
}

static inline uint8_t get_port_value(uint8_t port) {
    switch (port) {
#if defined(PCINT_INPUT_PORT0)
        case 0: return PCINT_INPUT_PORT0;
#endif
#if defined(PCINT_INPUT_PORT1)
        case 1: return PCINT_INPUT_PORT1;
#endif
#if defined(PCINT_INPUT_PORT2)
        case 2: return PCINT_INPUT_PORT2;
#endif
#if defined(PCINT_INPUT_PORT3)
        case 3: return PCINT_INPUT_PORT3;
#endif
        default: return 0;
    }
}

static inline uint8_t bitpos(uint8_t mask)
{
  for (uint8_t i = 0; i < 8; ++i) {
    if (mask & _BV(i)) {
      return i;
    }
  }
  return -1; //Should not happen
}

void PcInt::attachInterrupt(uint8_t pin, callback func, uint8_t mode) 
{
  //On AVR's default calling convention, if we call a no-arg funcion passing an argument,
  //it is silently ignored and nothing goes wrong.
  attachInterrupt(pin, (callback_arg)func, nullptr, mode);
}

void PcInt::attachInterrupt(uint8_t pin, callback_arg func, void* arg, uint8_t mode)
{
  volatile uint8_t * pcicr = digitalPinToPCICR(pin);
  volatile uint8_t * pcmsk = digitalPinToPCMSK(pin);
  if (pcicr && pcmsk) {
    uint8_t pcintGroup = digitalPinToPCICRbit(pin);
    uint8_t portBitMask = digitalPinToBitMask(pin);
    PcIntPort* port = get_port(pcintGroup);

    if (port) {
      port->funcs[bitpos(portBitMask)] = func;
      port->args[bitpos(portBitMask)] = arg;
      port->rising  |= (mode == RISING || mode == CHANGE) ? portBitMask : 0;
      port->falling |= (mode == FALLING || mode == CHANGE) ? portBitMask : 0;
      port->state    = get_port_value(pcintGroup);
      *pcmsk |= _BV(digitalPinToPCMSKbit(pin));
      *pcicr |= _BV(digitalPinToPCICRbit(pin));
    }
  }
}

void PcInt::detachInterrupt(uint8_t pin)
{
  volatile uint8_t * pcicr = digitalPinToPCICR(pin);
  volatile uint8_t * pcmsk = digitalPinToPCMSK(pin);
  if (pcicr && pcmsk) {
    uint8_t pcintGroup = digitalPinToPCICRbit(pin);
    uint8_t portBitMask = digitalPinToBitMask(pin);
    PcIntPort* port = get_port(pcintGroup);
    
    if (port) {
      port->funcs[bitpos(portBitMask)] = nullptr;
      port->args[bitpos(portBitMask)] = nullptr;
      port->rising &= ~portBitMask;
      port->falling &= ~portBitMask; 

      *pcmsk &= ~_BV(digitalPinToPCMSKbit(pin));
      //Switch off the group if all of the group are now off
      if (*pcmsk == 0x00F){ //Alternatively "if (!*pcmsk)"
        *pcicr &= ~_BV(digitalPinToPCICRbit(pin));
      }
    }
  }
}

void PcInt::enableInterrupt(uint8_t pin)
{
  volatile uint8_t * pcmsk = digitalPinToPCMSK(pin);
  if (pcmsk) {
    *pcmsk |= _BV(digitalPinToPCMSKbit(pin));
  }
}

void PcInt::disableInterrupt(uint8_t pin)
{
  volatile uint8_t * pcmsk = digitalPinToPCMSK(pin);
  if (pcmsk) {
    *pcmsk &= ~_BV(digitalPinToPCMSKbit(pin));
  }
}

/*
 * Get the installed function pointer
 *
 * This function serves just for diagnostic purposes.
 */
PcInt::callback PcInt::getFunc(uint8_t group, uint8_t nr)
{
  PcIntPort* port = get_port(group);
  if (port && nr < 8) {
    return (callback)port->funcs[nr];
  } else {
    return nullptr;
  }
}

#define IMPLEMENT_ISR(vect, port, pin_register) \
  ISR(vect) \
  { \
    uint8_t new_state = pin_register; \
    uint8_t trigger_pins = (port.state ^ new_state) & ( (port.rising & new_state) | (port.falling & ~new_state) ); \
    port.state = new_state; \
    for (uint8_t nr = 0; nr < 8; ++nr) { \
      if ((trigger_pins & _BV(nr)) && port.funcs[nr]) { \
        (*port.funcs[nr])(port.args[nr], bool(_BV(nr) & new_state)); \
      } \
    } \
  }

#if defined(PCINT_INPUT_PORT0)
IMPLEMENT_ISR(PCINT0_vect, port0, PCINT_INPUT_PORT0)
#endif

#if defined(PCINT_INPUT_PORT1)
IMPLEMENT_ISR(PCINT1_vect, port1, PCINT_INPUT_PORT1)
#endif

#if defined(PCINT_INPUT_PORT2)
IMPLEMENT_ISR(PCINT2_vect, port2, PCINT_INPUT_PORT2)
#endif

#if defined(PCINT_INPUT_PORT3)
IMPLEMENT_ISR(PCINT3_vect, port3, PCINT_INPUT_PORT3)
#endif
