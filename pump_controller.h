#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

void initPump();
void activatePump();
void deactivatePump();
void openValve();
void closeValve();
unsigned long getPumpSwitchOnTime();
unsigned long getPumpSwitchOffTime();

#endif
