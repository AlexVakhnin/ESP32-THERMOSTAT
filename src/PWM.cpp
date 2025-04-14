#include <Arduino.h>

#define HEATER_INTERVAL 1000
#define HEAT_RELAY_PIN  19

extern unsigned long time_now;
//extern unsigned long time_last;

int target_val = 0;
int current_val = 0;


float heatcycles;
bool heaterState = 0;
unsigned long heatCurrentTime = 0, heatLastTime = 0; 

void pwm_setup() {
    pinMode(HEAT_RELAY_PIN, OUTPUT);
}

void _turnHeatElementOnOff(bool on) {
    digitalWrite(HEAT_RELAY_PIN, on);
    heaterState = on;
}

void pwm_handle() {
  heatCurrentTime = time_now;
  bool newStatus = false;
  bool shouldUpdate = false;
  if(heatCurrentTime - heatLastTime >= HEATER_INTERVAL or heatLastTime > heatCurrentTime) { //second statement prevents overflow errors
    // begin cycle
    newStatus = true;
    shouldUpdate = true;
    heatLastTime = heatCurrentTime;
  }
  if (heatCurrentTime - heatLastTime >= heatcycles) {
    shouldUpdate = true;
    newStatus = false;
  }
  if(shouldUpdate){
    _turnHeatElementOnOff(newStatus);
  }
}

void setHeatPowerPercentage(float power) {
  if (power < 0.0) {
    power = 0.0;
  }  
  if (power > 1000.0) {
    power = 1000.0;
  }
  heatcycles = power;
}

float getHeatCycles() {
  return heatcycles;
}
