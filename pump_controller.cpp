#include "pump_controller.h"
#include <Arduino.h>
#include "config.h"
#include "network_manager.h"

static unsigned long pumpSwitchOnTime = 0;
static unsigned long pumpSwitchOffTime = 0;
static bool pumpRunning = false;

void initPump() {
  pinMode(pumpPin, OUTPUT);
  pinMode(valvePin, OUTPUT);
  digitalWrite(pumpPin, LOW);
}

void openValve() {
  Serial.println("Valve OPEN");
  digitalWrite(valvePin, HIGH);
  pumpSwitchOnTime = millis() + valveDuration * 1000;
}

void activatePump() {
  Serial.println("Pump ON");
  digitalWrite(pumpPin, HIGH);
  pumpRunning = true;
  pumpSwitchOnTime = 0;
  pumpSwitchOffTime = millis() + pumpDuration * 1000;
  sendPumpStatus(true);
}

unsigned long getPumpSwitchOnTime() {
  return pumpSwitchOnTime;
}

unsigned long getPumpSwitchOffTime() {
  return pumpSwitchOffTime;
}

void closeValve() {
  Serial.println("Valve CLOSED");
  digitalWrite(valvePin, LOW);
}

void deactivatePump() {
  digitalWrite(pumpPin, LOW);
  pumpSwitchOnTime = 0;  // cancel scheduled pump activation
  pumpSwitchOffTime = 0;
  if (!pumpRunning) return;
  pumpRunning = false;
  Serial.println("Pump OFF");
  sendPumpStatus(false);
}
