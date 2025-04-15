#include <Arduino.h>

#define PWM_PERIOD 1000  //период PWM алгоритма
#define RELAY_PIN  2 //порт управления реле

extern unsigned long time_now;
//extern int counter;
//extern unsigned long time_last;

int target_val = 0;
int current_val = 0;

float heatcycles; //время в милисекундах верхней полки PWM (0-1000) 
bool relayState = 0; //содержит текущее состояние реле (0/1)
unsigned long heatCurrentTime = 0;  //для вычисления интервалов PWM
unsigned long heatLastTime = 0;  //для вычисления интервалов PWM

void pwm_setup() {
    pinMode(RELAY_PIN, OUTPUT); 
    digitalWrite(RELAY_PIN, false);
}

//меняем состояние нагревателя 
void _turnHeatElementOnOff(bool state) {
    digitalWrite(RELAY_PIN, state);
    relayState = state;
    //Serial.println("Set Relay="+String(state));
}
//---------------------------------------------LOOP--------------------------------------
void pwm_handle() {
  heatCurrentTime = time_now;
  bool newStatus = false;
  bool shouldUpdate = false;

  //проверка выхода за временные пределы периода PWM
  if(heatCurrentTime - heatLastTime >= PWM_PERIOD or heatLastTime > heatCurrentTime) { //второй оператор предотвращает ошибки переполнения
    // начинаем новый период PWM !!!
    shouldUpdate = true; //нужно обновить
    newStatus = true;  //обновить - включить
    Serial.println("__/-- "+String(PWM_PERIOD)+" "+String(heatCurrentTime)+"-"+String(heatLastTime)+"="+String(heatCurrentTime - heatLastTime));
    heatLastTime = heatCurrentTime;
  }
/*
  // проверка выхода за временные пределы верхней (горячей) полки PWM
  if (heatCurrentTime - heatLastTime >= heatcycles && shouldUpdate==false && heaterState==1) {
    shouldUpdate = true; //нужно обновить
    newStatus = false;  //обновить - выключить
    //delay(20);
    Serial.println("--\\__ "+String(heatcycles)+" "+String(heatCurrentTime)+"-"+String(heatLastTime)+"="+String(heatCurrentTime - heatLastTime));
  }
*/
  //меняем состояние выхода PWM (0/1)
  if(shouldUpdate){
    _turnHeatElementOnOff(newStatus);
    shouldUpdate=false;
  }
}
//---------------------------------------------END LOOP--------------------------------

//Устанавливаем мощность нагрева (0-1000)
void setHeatPowerPercentage(float power) {
  if (power < 0.0) {
    power = 0.0;
  }  
  if (power > 1000.0) {
    power = 1000.0;
  }
  //установка мощности нагрева в милисекундах !!! (0-1000)
  heatcycles = power;
}

float getHeatCycles() {
  return heatcycles;
}
