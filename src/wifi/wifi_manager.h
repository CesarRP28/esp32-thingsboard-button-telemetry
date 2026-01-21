#pragma once
#include <Arduino.h>

namespace wifi {
  void connect(uint32_t timeoutMs = 15000, uint32_t retryDelayMs = 500);
  bool isConnected();
  String ip();
}
