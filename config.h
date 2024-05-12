#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Sensor and Pump Pins
const int sensorPin = A0;
const int pumpPin = 8;

// Operational Parameters
extern const int configuration_id;
extern volatile int moistureThreshold;
extern volatile int measurementInterval; // in seconds
extern volatile int pumpDuration;     // in seconds
extern volatile int consecutivePumps;
extern volatile int maxConsecutivePumps; // number of times pump may run consecutively, otherwise it's disabled
extern volatile bool measurementEnabled;
extern volatile bool pumpEnabled;

#endif
