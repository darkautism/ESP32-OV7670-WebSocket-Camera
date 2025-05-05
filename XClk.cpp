//parts of his code are taken from
//https://github.com/igrr/esp32-cam-demo
//by Ivan Grokhotkov
//released under Apache License 2.0

#include "XClk.h"
#include "driver/ledc.h"

bool ClockEnable(int pin, int Hz) {
  uint8_t resolution = 1;
  if (!ledcAttach(pin, Hz, 1)) {
    Serial.println("ledcAttach failed");
    return false;
  }
  uint32_t duty = (1 << (resolution - 1));  // 設置 50% 占空比
  ledcWrite(pin, duty);
  return true;
}

void ClockDisable(int pin) {
  ledcDetach(pin);
}
