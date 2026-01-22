#pragma once
#include <Arduino.h>

namespace tb_mqtt {
  void begin();
  void loop();
  bool isConnected();
}
