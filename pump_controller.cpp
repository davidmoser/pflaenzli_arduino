#include "pump_controller.h"
#include <Arduino.h>
#include "config.h"
#include "network_manager.h"

static bool pumpOn = false;
static unsigned long pumpSwitchOffTime = 0;

void initPump() {
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);
}

void activatePump() {
  Serial.println("Pump ON");
  digitalWrite(pumpPin, HIGH);
  pumpSwitchOffTime = millis() + pumpDuration * 1000;
  pumpOn = true;
  sendPumpStatus(true);
}

unsigned long getPumpSwitchOffTime() {
  return pumpSwitchOffTime;
}

void deactivatePump() {
  if(!pumpOn) return;
  Serial.println("Pump OFF");
  digitalWrite(pumpPin, LOW);
  pumpOn = false;
  sendPumpStatus(false);
}
