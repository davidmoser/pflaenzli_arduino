#include "config.h"
#include "network_manager.h"
#include "sensor_manager.h"
#include "pump_controller.h"
#include "command_handler.h"
#include "WDT.h"

static unsigned long reconnectTime = 0;

void setup() {
  Serial.begin(9600);
  WDT.begin(5000);  // hardware watchdog: if the loop ever hangs, reset & recover instead of going silently dead
  setupNetwork();
  retrieveConfiguration();
  initSensor();
  initPump();
  Serial.println("Setup complete.");
}

void loop() {
  WDT.refresh();  // kick the watchdog each iteration; a stalled loop will miss this and trigger a reset
  unsigned long currentTime = millis();

  // Check if it's time to read the sensor
  if (measurementEnabled && currentTime >= getNextMeasurementTime()) {
    readMoisture();
  }

  if (getPumpSwitchOnTime() != 0 && currentTime >= getPumpSwitchOnTime()) {
    activatePump();
  }

  if (getPumpSwitchOffTime() != 0 && currentTime >= getPumpSwitchOffTime()) {
    deactivatePump();
    closeValve();
  }

  // Always check for commands
  checkForCommands();

  // Check network connection and reconnect
  if (currentTime >= reconnectTime) {
    reconnectTime = currentTime + 60 * 1000;
    reconnectNetwork();
    retrieveConfiguration();
  }

  delay(10);
}

//TODO: Reload parameter configuration from remote server on setupNetwork
