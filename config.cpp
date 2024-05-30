#include "config.h"

// Operational Parameters
const int configuration_id = 1;          // id of configuration record on server
volatile int moistureThreshold = 5;
volatile int measurementInterval = 600;  // in seconds
volatile int pumpDuration = 120;         // in seconds
volatile int valveDuration = 7;          // in seconds
volatile int consecutivePumps = 0;       // number of times pump has run after consecutive measurements
volatile int maxConsecutivePumps = 5;    // number of times pump may run consecutively, otherwise it's disabled
volatile bool measurementEnabled = true;
volatile bool pumpEnabled = true;
