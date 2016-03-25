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
#include <avr/interrupt.h>

class PcIntPort
{
public:
  void (*funcs[8])(void);
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


#if defined(PCINT0_vect)
PcIntPort port0;
#endif
#if defined(PCINT1_vect)
PcIntPort port1;
#endif
#if defined(PCINT2_vect)
PcIntPort port2;
#endif
#if defined(PCINT3_vect)
PcIntPort port3;
#endif

/*
 * Set the function pointer in the array using the port's pin bit mask
 */
static void setFunc(void (*funcs[])(void), uint8_t portBitMask, void (*func)(void))
{
  for (uint8_t i = 0; i < 8; ++i) {
    if (portBitMask & 1) {
      funcs[i] = func;
      break;
    }
    portBitMask >>= 1;
  }
}

void PcInt::attachInterrupt(uint8_t pin, void(*func)(void), uint8_t mode)
{
  volatile uint8_t * pcicr = digitalPinToPCICR(pin);
  volatile uint8_t * pcmsk = digitalPinToPCMSK(pin);
  if (pcicr && pcmsk) {
    uint8_t pcintGroup = digitalPinToPCICRbit(pin);
    uint8_t portBitMask = digitalPinToBitMask(pin);
    switch (pcintGroup) {
#if defined(PCINT0_vect)
    case 0:
      setFunc(port0.funcs, portBitMask, func);
      port0.rising |= (mode == RISING || mode == CHANGE) ? portBitMask : 0;
      port0.falling |= (mode == FALLING || mode == CHANGE) ? portBitMask : 0;
      port0.state = PINA;
      break;
#endif
#if defined(PCINT1_vect)
    case 1:
      setFunc(port1.funcs, portBitMask, func);
      port1.rising |= (mode == RISING || mode == CHANGE) ? portBitMask : 0;
      port1.falling |= (mode == FALLING || mode == CHANGE) ? portBitMask : 0;
      port1.state = PINB;
      break;
#endif
#if defined(PCINT2_vect)
    case 2:
      setFunc(port2.funcs, portBitMask, func);
      port2.rising |= (mode == RISING || mode == CHANGE) ? portBitMask : 0;
      port2.falling |= (mode == FALLING || mode == CHANGE) ? portBitMask : 0;
      port2.state = PINC;
      break;
#endif
#if defined(PCINT3_vect)
    case 3:
      setFunc(port3.funcs, portBitMask, func);
      port3.rising |= (mode == RISING || mode == CHANGE) ? portBitMask : 0;
      port3.falling |= (mode == FALLING || mode == CHANGE) ? portBitMask : 0;
      port3.state = PIND;
      break;
#endif
    }
    *pcmsk |= _BV(digitalPinToPCMSKbit(pin));
    *pcicr |= _BV(digitalPinToPCICRbit(pin));
  }
}

void PcInt::detachInterrupt(uint8_t pin)
{
  volatile uint8_t * pcicr = digitalPinToPCICR(pin);
  volatile uint8_t * pcmsk = digitalPinToPCMSK(pin);
  if (pcicr && pcmsk) {
    uint8_t pcintGroup = digitalPinToPCICRbit(pin);
    uint8_t portBitMask = digitalPinToBitMask(pin);
    switch (pcintGroup) {
#if defined(PCINT0_vect)
    case 0:
      setFunc(port0.funcs, portBitMask, NULL);
      port0.rising &= ~portBitMask;
      port0.falling &= ~portBitMask; 
      break;
#endif
#if defined(PCINT1_vect)
    case 1:
      setFunc(port1.funcs, portBitMask, NULL);
      port1.rising &= ~portBitMask;
      port1.falling &= ~portBitMask;
      break;
#endif
#if defined(PCINT2_vect)
    case 2:
      setFunc(port2.funcs, portBitMask, NULL);
      port2.rising &= ~portBitMask;
      port2.falling &= ~portBitMask;
      break;
#endif
#if defined(PCINT3_vect)
    case 3:
      setFunc(port3.funcs, portBitMask, NULL);
      port3.rising &= ~portBitMask;
      port3.falling &= ~portBitMask;
      break;
#endif
    }
    *pcmsk &= ~_BV(digitalPinToPCMSKbit(pin));
    //Switch off the group if all of the group are now off
    if (*pcmsk == 0x00F){ //Alternatively "if (!*pcmsk)"
      *pcicr &= ~_BV(digitalPinToPCICRbit(pin));
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
void (*PcInt::getFunc(uint8_t group, uint8_t nr))(void)
{
  if (nr >= 8) {
    return 0;
  }
  void   (**funcs)(void);
  switch (group) {
  case 0:
    funcs = port0.funcs;
    break;
#if defined(PCINT1_vect)
  case 1:
    funcs = port1.funcs;
    break;
#endif
#if defined(PCINT2_vect)
  case 2:
    funcs = port2.funcs;
    break;
#endif
#if defined(PCINT3_vect)
  case 3:
    funcs = port3.funcs;
    break;
#endif
  default:
    return 0;
    break;
  }
  return funcs[nr];
}

#if defined(PCINT0_vect)
inline void PcInt::handlePCINT0()
{
  uint8_t changedPins = port0.state ^ PINA;
  port0.state = PINA;
  for (uint8_t nr = 0; nr < 8; ++nr) {
    if (changedPins & _BV(nr)) {
      if (((_BV(nr) & port0.rising & port0.state) | (_BV(nr) & port0.falling & ~port0.state))) {
        if (port0.funcs[nr]) {
          (*port0.funcs[nr])();
        }
      }
    }
  }
}
ISR(PCINT0_vect)
{
  PcInt::handlePCINT0();
}
#endif

#if defined(PCINT1_vect)
inline void PcInt::handlePCINT1()
{
  uint8_t changedPins = port1.state ^ PINB;
  port1.state = PINB;
  for (uint8_t nr = 0; nr < 8; ++nr) {
    if (changedPins & _BV(nr)) {
      if (((_BV(nr) & port1.rising & port1.state) | (_BV(nr) & port1.falling & ~port1.state))) {
        if (port1.funcs[nr]) {
          (*port1.funcs[nr])();
        }
      }
    }
  }
}
ISR(PCINT1_vect)
{
  PcInt::handlePCINT1();
}
#endif

#if defined(PCINT2_vect)
inline void PcInt::handlePCINT2()
{
  uint8_t changedPins = port2.state ^ PINC;
  port2.state = PINC;
  for (uint8_t nr = 0; nr < 8; ++nr) {
    if (changedPins & _BV(nr)) {
      if (((_BV(nr) & port2.rising & port2.state) | (_BV(nr) & port2.falling & ~port2.state))) {
        if (port2.funcs[nr]) {
          (*port2.funcs[nr])();
        }
      }
    }
  }
}
ISR(PCINT2_vect)
{
  PcInt::handlePCINT2();
}
#endif

#if defined(PCINT3_vect)
inline void PcInt::handlePCINT3()
{
  uint8_t changedPins = port3.state ^ PIND;
  port3.state = PIND;
  for (uint8_t nr = 0; nr < 8; ++nr) {
    if (changedPins & _BV(nr)) {
      if (((_BV(nr) & port3.rising & port3.state) | (_BV(nr) & port3.falling & ~port3.state))) {
        if (port3.funcs[nr]) {
          (*port3.funcs[nr])();
        }
      }
    }
  }
  
}
ISR(PCINT3_vect)
{
  PcInt::handlePCINT3();
}
#endif
