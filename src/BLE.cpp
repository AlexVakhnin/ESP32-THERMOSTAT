#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

String dev_name;
extern int target_val;
extern int current_val;
extern float pid_kp;
extern float pid_ki;
extern float pid_kd;
extern void disp_val(int val);

void storage_dev_name(String dname);
void set_target(String targ);
void storage_pid_kp(String su);
void storage_pid_ki(String su);
void storage_pid_kd(String su);
void help_print();
void reset_nvram();

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
uint32_t value = 0;

Preferences preferences; //for NVRAM

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "1418a369-a491-48a9-a9cb-05a607f23665"
#define CHARACTERISTIC_UUID "751318ac-9022-4431-81c8-a3eae227bcba"

void ble_handle_tx(String str);

//ловим события connect/disconnect от BLE сервера
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
      //формируем адрес клиента
      char remoteAddress[18];
      sprintf( remoteAddress , "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
      param->connect.remote_bda[0],
      param->connect.remote_bda[1],
      param->connect.remote_bda[2],
      param->connect.remote_bda[3],
      param->connect.remote_bda[4],
      param->connect.remote_bda[5]
      );
      //фильтруем mac адреса..
      //if (param->connect.remote_bda[0]==0x34){
      //    pServer->disconnect(param->connect.conn_id) ;//force disconnect client..
      //}
      Serial.print("Event-Connect..");Serial.println(remoteAddress);
      deviceConnected = true;
      //digitalWrite(8, LOW); //internal led = ON (DEBUG..)
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Event-Disconnect..");
      delay(300); // give the bluetooth stack the chance to get things ready
      BLEDevice::startAdvertising();  // restart advertising
      //digitalWrite(8, HIGH); //internal led = OFF (DEBUG..)
    }
};

//ловим события write от BLE сервиса
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param) { //запись в сервис..
        std::string rxValue = pCharacteristic->getValue(); //строка от BLE

        if (rxValue.length() > 0) {
            //печатаем строку от BLE
            String pstr = String(rxValue.c_str());
            Serial.print("--BLE: received: ");Serial.print(pstr);
            
            // Do stuff based on the command received from the app
            if (pstr=="at"||pstr=="at\r\n") {     //at
                ble_handle_tx("OK\r\n"); //sensor number
            }
            else if (pstr=="at?"||pstr=="at?\r\n") { //at? - help
                help_print();
            }            
            else if (pstr=="atz"||pstr=="atz\r\n") { //atz - reset NVRAM
                reset_nvram();
            }            
            else if (pstr=="ati"||pstr=="ati\r\n") { //ati - information
              String s ="DeviceName: "+dev_name
                  +"\r\nFreeMem: "+String(ESP.getFreeHeap())
                  +"\r\nkp="+String(pid_kp)
                  +"\r\nki="+String(pid_ki)
                  +"\r\nkd="+String(pid_kd)
                  +"\r\n--------------state---------------"
                  +"\r\ntarget_val="+String(target_val)
                  +"\r\ncurrent_val="+String(current_val);
                ble_handle_tx(s+"\r\n"); //send string to BLE client
            }

            else if (pstr.substring(0,4)=="atn=") { //atn= - dev_name save NVRAM
                storage_dev_name(pstr.substring(4)); //dev_name
            }

            else if (pstr.substring(0,5)=="atkp=") { //pid_kp
                storage_pid_kp(pstr.substring(5));
            }
            else if (pstr.substring(0,5)=="atki=") { //pid_ki
                storage_pid_ki(pstr.substring(5));
            }
            else if (pstr.substring(0,5)=="atkd=") { //pid_kd
                storage_pid_kd(pstr.substring(5));
            }

           //первый символ-цифра (48-57)
            else if (pstr.substring(0,1)>="0" && pstr.substring(0,1)<="9") { //atw= - winter_temp save NVRAM
                set_target(pstr);
            }
            
            else {ble_handle_tx("???\r\n");}            
        }
    }
 };

//Init BLE Service
void ble_setup(){

  //читаем все параметры NVRAM
  preferences.begin("EspNvram", true); //открываем пространство имен NVRAM read only

  dev_name = preferences.getString("dev_name", "ESP32-THERMOSTAT");
  pid_kp = preferences.getFloat("pid_kp", 20.73);
  pid_ki = preferences.getFloat("pid_ki", 11.5);
  pid_kd = preferences.getFloat("pid_kd", 1);

  preferences.end(); //закрываем NVRAM

  // Create the BLE Device
  BLEDevice::init(dev_name.c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); //Обработчик событий connect/disconnect от BLE

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  // все свойства характеристики (READ,WRITE..)- относительно клиента !!!
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ      |
                      BLECharacteristic::PROPERTY_WRITE     |
                      BLECharacteristic::PROPERTY_NOTIFY    |
                      BLECharacteristic::PROPERTY_INDICATE  |
                      BLECharacteristic::PROPERTY_WRITE_NR  
                    );

  // Create a BLE Descriptor !!!
  pCharacteristic->addDescriptor(new BLE2902()); //без дескриптора не подключается..!!!

  pCharacteristic->setCallbacks(new MyCallbacks()); //обработчик событий read/write

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); 
  pAdvertising->addServiceUUID(SERVICE_UUID);  //рекламируем свой SERVICE_UUID(0x07)
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);//Conn.Intrval(0x12) is:0x06 [5 0x12 0x06004000] iOS..
  //pAdvertising->setMinPreferred(0x12);//Conn.Intrval(0x12) is:0x12 [5 0x12 0x12004000] Default
  //pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();  
  
  Serial.print("BLE Server address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
  Serial.println("BLE Device name: "+ dev_name);
}

//отправляем строку клиенту BLE на терминал.. 
void ble_handle_tx(String str){
    if (deviceConnected) { //проверка, что подключен клиент BLE
        if(str.length()==0) str="none..\r\n";
        pCharacteristic->setValue(str.c_str());
        pCharacteristic->notify(false); //false=indicate; true=wait confirmation
    }
}

//сохранить имя BLE устройства в NVRAM
void storage_dev_name(String dname){ 
  int n = dname.length(); 
  dev_name = dname.substring(0,n-2); //удалить \r\n в конце строки..
  ble_handle_tx("new dev_name="+dev_name+"\r\n"); //ответ на BLE
  preferences.begin("EspNvram", false);
  preferences.putString("dev_name", dev_name);
  preferences.end();
  delay(3000);
  ESP.restart();
}

void storage_pid_kp(String su){
    pid_kp = su.toFloat();
    ble_handle_tx("new pid_kp="+String(pid_kp)); //ответ на BLE
    preferences.begin("EspNvram", false);
    preferences.putFloat("pid_kp", pid_kp);
    preferences.end();
  }
  void storage_pid_ki(String su){
    pid_ki = su.toFloat();
    ble_handle_tx("new pid_ki="+String(pid_ki)); //ответ на BLE
    preferences.begin("EspNvram", false);
    preferences.putFloat("pid_ki", pid_ki);
    preferences.end();
  }
  void storage_pid_kd(String su){
    pid_kd = su.toFloat();
    ble_handle_tx("new pid_kd="+String(pid_kd)); //ответ на BLE
    preferences.begin("EspNvram", false);
    preferences.putFloat("pid_kd", pid_kd);
    preferences.end();
  }
      


void set_target(String targ){

    int n=1; //количество цифр в начале строки..

    if(targ.substring(1,2)>="0" && targ.substring(1,2)<="9"){ //второй знак - цифра
        n++;
        if(targ.substring(2,3)>="0" && targ.substring(2,3)<="9"){ //третий знак - цифра
            n++;
        }
    }
    String str = targ.substring(0,n);
    Serial.println("Target String= "+ str);
    target_val = str.toInt();
    disp_val(target_val);
} 


//команда at?
void help_print(){
  String  shelp="ati - list current state";
        shelp+="\r\natn=[name] - BLE device name";
        shelp+="\r\natkp=[pid_kp] - pid kp";
        shelp+="\r\natki=[pid_ki] - pid ki";
        shelp+="\r\natkd=[pid_kd] - pid kd";
        shelp+="\r\n[target_temp] - target temperature";
        shelp+="\r\natz - set to default";
  ble_handle_tx(shelp+"\r\n");
}

//команда atz - очистить NVRAM
void reset_nvram(){
  preferences.begin("EspNvram", false); //пространство имен (чтение-запись)
  preferences.clear(); //удаляем все ключи в пространстве имен
  preferences.end();
  ble_handle_tx("NVRAM Key Reset..\r\n"); //ответ на BLE
  delay(3000);
  ESP.restart();
}
