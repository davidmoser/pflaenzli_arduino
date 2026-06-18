#include "config.h"

// Operational Parameters
const int configuration_id = 1;          // id of configuration record on server
volatile int measurementInterval = 600;  // in seconds
volatile int pumpDuration = 5;         // in seconds
volatile int valveDuration = 7;          // in seconds
volatile bool measurementEnabled = true;
volatile bool pumpEnabled = true;
