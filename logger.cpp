#include "logger.h"

static String pad(unsigned long v, int width) {
  String s(v);
  while ((int)s.length() < width) s = "0" + s;
  return s;
}

void logInfo(const String& msg) {
  unsigned long ms = millis();
  unsigned long s = ms / 1000;
  Serial.println("[" + pad(s / 3600, 2) + ":" + pad(s / 60 % 60, 2) + ":" + pad(s % 60, 2) + "." +
                 pad(ms % 1000, 3) + "] " + msg);
}

void logDebug(const String& msg) {
#if DEBUG_LOG
  logInfo(msg);
#endif
}
