#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Sensor and Pump Pins
const int sensorPin = A0;
const int pumpPin = 8;
const int valvePin = 7;

// Operational Parameters
extern const int configuration_id;       // id of the single configuration record in the backend
extern volatile int moistureThreshold;   // if moisture is below threshold the pump is started
extern volatile int measurementInterval; // time between measurements in seconds
extern volatile int pumpDuration;        // how long the pump pumps in seconds
extern volatile int valveDuration;       // how long the valve takes to open in seconds
extern volatile int consecutivePumps;    // current number of consecutive pumps
extern volatile int maxConsecutivePumps; // number of times pump may run consecutively, otherwise it's disabled
extern volatile bool measurementEnabled; // switch to enable/disable automatic measurements
extern volatile bool pumpEnabled;        // switch to enable/disable automatic pumps

#endif
