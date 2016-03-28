/*
 * Sodaq_PcInt.h
 *
 * This module supplies a set of helper functions to use the
 * PinChange interrupt in a convenient manner, similar to
 * the standard Arduino attachInterrupt.
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
 */

#ifndef SODAQ_PCINT_H_
#define SODAQ_PCINT_H_

#include <Arduino.h>
#include <stdint.h>

class PcInt
{
public:
  typedef void (*callback)(void);
  typedef void (*callback_arg)(void *arg, bool value);
  
  static void attachInterrupt(uint8_t pin, callback func, uint8_t mode=CHANGE);
  static void attachInterrupt(uint8_t pin, callback_arg func, void *arg, uint8_t mode=CHANGE);
  template<typename T> 
  static inline void attachInterrupt(uint8_t pin, void(*func)(T *arg, bool value), T *arg, uint8_t mode=CHANGE) {
    attachInterrupt(pin, (PcInt::callback_arg)func, (void*) arg, mode);
  }
  static void detachInterrupt(uint8_t pin);
  static void enableInterrupt(uint8_t pin);
  static void disableInterrupt(uint8_t pin);

  // For diagnostic purposes
  static callback getFunc(uint8_t group, uint8_t nr);
};

#endif /* SODAQ_PCINT_H_ */
