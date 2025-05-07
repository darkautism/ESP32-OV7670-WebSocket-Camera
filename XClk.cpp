// Author : Kautism<darkautism@gmail.com>

#include "XClk.h"
#include "Log.h"

#include "driver/ledc.h"

bool ClockEnable(int pin, int Hz) {
  uint8_t resolution = 1;
  if (!ledcAttach(pin, Hz, 1)) {
    DEBUG_PRINTLN("ledcAttach failed");
    return false;
  }
  uint32_t duty = (1 << (resolution - 1));
  ledcWrite(pin, duty);
  return true;
}

void ClockDisable(int pin) { ledcDetach(pin); }
