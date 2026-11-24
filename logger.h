#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#define DEBUG_LOG 1  // 1: verbose serial diagnostics, 0: normal operation

void logInfo(const String& msg);
void logDebug(const String& msg);

#endif
