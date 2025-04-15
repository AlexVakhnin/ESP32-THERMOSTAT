#include <Arduino.h>

#define PID_INTERVAL 1000//200  //(в милисекундах) период  для PID алгоритма

extern void disp_setup();
extern void state_logo();
extern void disp_val(int val);
extern void ble_setup();
extern void pwm_setup();
extern void pwm_handle();
extern void setHeatPowerPercentage(float power);

extern int target_val;
extern float heatcycles;

long time_now=0; //текущее время в цикле
long time_last=0; //хранит аремя для периодического события PID алгоритма

int counter = 0;



void setup() {
  Serial.begin(115200);

  disp_setup();
  ble_setup(); //start BLE server + load from NVRAM
  pwm_setup();

  time_now=millis();
  time_last=time_now;
}

//программно реализуем PWM..
void loop() {
  time_now=millis();


  if((time_now-time_last)>=PID_INTERVAL or time_last > time_now) { //обработка PID алгоритма
    /*
    if( !overShootMode && abs(gTargetTemp-currentTemp)>=gOvershoot ) {        
      ESPPID.SetTunings(gaP, gaI, gaD);
      overShootMode=true;
    }
    else if( overShootMode && abs(gTargetTemp-currentTemp)<gOvershoot ) {
      ESPPID.SetTunings(gP,gI,gD);
      overShootMode=false;
    }

    if(ESPPID.Compute()==true) {   //если вычислено новое значение PWM
      setHeatPowerPercentage(gOutputPwr); //Устанавливаем мощность нагрева (0-1000)
    }
    */
    setHeatPowerPercentage(target_val);  //тест - меандр..
    time_last=time_now;
    //Serial.println("Event-PID Computing.."+String(counter++)+" "+String(heatcycles));
  }


  pwm_handle();
}

