#include "sensor_manager.h"
#include "network_manager.h"
#include "config.h"

static unsigned long nextMeasurementTime = 0;

unsigned long getNextMeasurementTime() {
  return nextMeasurementTime;
}

void initSensor() {
  pinMode(sensorPin, INPUT);
}

float readMoisture() {
  int moisture = analogRead(sensorPin) / 1023.0 * 100;
  Serial.println("Moisture: " + String(moisture));
  nextMeasurementTime = millis() + measurementInterval * 1000;
  sendSensorData(moisture);  // Send data to the server right after measurement
  return moisture;  // Convert to percentage
}

void triggerMeasurement() {
  measurementEnabled = true;
  nextMeasurementTime = 0;
}
