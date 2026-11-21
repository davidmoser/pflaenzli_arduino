#include "config.h"
#include "network_manager.h"
#include "sensor_manager.h"
#include "pump_controller.h"
#include "command_handler.h"
#include "WDT.h"

static unsigned long reconnectTime = 0;

void logResetCause() {
  uint8_t rstsr0 = R_SYSTEM->RSTSR0;
  uint16_t rstsr1 = R_SYSTEM->RSTSR1;
  bool warmStart = R_SYSTEM->RSTSR2_b.CWSF;

  String cause;
  if (R_SYSTEM->RSTSR0_b.PORF) cause = "power-on";
  else if (R_SYSTEM->RSTSR0_b.LVD0RF || R_SYSTEM->RSTSR0_b.LVD1RF || R_SYSTEM->RSTSR0_b.LVD2RF) cause = "low voltage (brownout)";
  else if (R_SYSTEM->RSTSR1_b.WDTRF || R_SYSTEM->RSTSR1_b.IWDTRF) cause = "watchdog";
  else if (R_SYSTEM->RSTSR1_b.SWRF) cause = "software reset";
  else if (warmStart) cause = "reset pin / button";
  else cause = "unknown";
  Serial.println("Reset cause: " + cause + " (RSTSR0=0x" + String(rstsr0, HEX) +
                 " RSTSR1=0x" + String(rstsr1, HEX) + " CWSF=" + String(warmStart) + ")");

  R_SYSTEM->RSTSR0 = 0;
  R_SYSTEM->RSTSR1 = 0;
  R_SYSTEM->RSTSR2_b.CWSF = 1;
}

void setup() {
  Serial.begin(9600);
  logResetCause();
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
    printWiFiStatus();
  }

  delay(10);
}

//TODO: Reload parameter configuration from remote server on setupNetwork
