#include <Arduino.h>
#include <U8g2lib.h>

void disp_val(int val);
void state_logo();


//I2C for SH1106 OLED 128x64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


void disp_setup(){

  //OLED SH1106 128x64
  u8g2.begin();
  state_logo();
  delay(2000);
  disp_val(0);
}

void state_logo(){
    u8g2.clearBuffer();					// clear display buffer
    u8g2.setFont(u8g2_font_open_iconic_embedded_4x_t);//open iconic
    u8g2.drawStr(48,64-15,"N");
    u8g2.sendBuffer();
  }
  
  
  //целое число - выдать на дисплей
  void disp_val(int val){
    String str_value ="---";
    if (val >=0 && val <= 999){  //проверяем..
      str_value = String(val);
      if (val>=0 && val <= 9) str_value = "  "+str_value;
      if (val>=10 && val <= 99) str_value = " "+str_value;
    }
    u8g2.clearBuffer();					// clear display buffer
    u8g2.setFont(u8g2_font_logisoso62_tn);//big font
    u8g2.drawStr(2,63,str_value.c_str());
    u8g2.sendBuffer();
  }
  