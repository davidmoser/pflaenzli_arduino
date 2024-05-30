#include "config.h"
#include "network_manager.h"
#include "sensor_manager.h"
#include "pump_controller.h"
#include "command_handler.h"

static unsigned long reconnectTime = 0;

void setup() {
  Serial.begin(9600);
  setupNetwork();
  retrieveConfiguration();
  initSensor();
  initPump();
  Serial.println("Setup complete.");
}

void loop() {
  unsigned long currentTime = millis();

  // Check if it's time to read the sensor
  if (measurementEnabled && currentTime >= getNextMeasurementTime()) {
    float moisture = readMoisture();
    if (pumpEnabled && moisture < moistureThreshold) {
      if (consecutivePumps < maxConsecutivePumps) {
        // Open the valve before starting the pump
        openValve();
        consecutivePumps++;
      }
    } else {
      consecutivePumps = 0;
    }
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
