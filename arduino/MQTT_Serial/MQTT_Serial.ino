#include <EEPROM.h>

const int modem_onoff_reset_pin = 2;
const int gate_on_pin = 3;
const int gate_off_pin = 4;
const int infrared_vcc1 = 8;
const int infrared_pin1 = A0;
const int infrared_vcc2 = 9;
const int infrared_pin2 = A1;
const int close_threshold = 1000;
const int open_threshold = 5;
int memory_length = 128;
int pub_time_term = 1 * 60; //(mean of 1 is one min)
int distance1 = 0;
int distance2 = 0;
unsigned long pre_time;
unsigned long previousMillis = 0;
boolean gate_status_open_lock = true;
boolean gate_status_close_lock = true;
boolean time_lock = true;
boolean io_mode = true;
boolean mqtt_ready = false;
String mqttip = "221.155.134.216";
String mqttport = "1883";
String username = "\"sangmin\"";
String password = "\"sangmin2019\"";
String sub_topic = "\"command/gate/";
String pub_topic = "\"status/gate/";
String modem_number;
String sender;

void Modem_reset(){
    mqtt_ready = false;
    if(io_mode == true){
        Serial.println("Called: Modem_reset()");
        Serial.println("Send AT-Command: AT+CNUM");
    }
    digitalWrite(modem_onoff_reset_pin, LOW);
    delay(200);
    digitalWrite(modem_onoff_reset_pin, HIGH);
}

void Modem_init(){
    if(io_mode == true){
        Serial.println("Called: Modem_init()");
        Serial.println("Send AT-Command: AT+CMGF=1");
    }
    sub_topic = "\"command/gate/";
    pub_topic = "\"status/gate/";
    Modem_sns_memory_reset();
    String data;
    byte count = 0;
    if (is_memory() == true){
        for(int index = 0; index < memory_length; index++){
            char c = char(EEPROM.read(index));
            data = String(data + c);
            if (c == 34){         //34 = "\""
                count++;
            }
            if (count == 2){
                break;
            }
        }
    }
    mqttip = data;
    Serial1.println("AT+CMGF=1");
}

void Modem_sns_memory_reset(){
    if(io_mode == true){
        Serial.println("Called: Modem_sns_memory_reset()");
        Serial.println("Send AT-Command: AT+CMGD=,1");
    }
    Serial1.println("AT+CMGD=,1");
}

void Modem_cnum(){
    if(io_mode == true){
        Serial.println("Called: Modem_cnum()");
        Serial.println("Send AT-Command: AT+CNUM");
    }
    Serial1.println("AT+CNUM");
}

void Modem_cmgr(String message){
    if(io_mode == true){
        Serial.println("Called: Modem_cmgr()");
        Serial.println("Send AT-Command: AT+CMGR");
    }
    int index = message.indexOf(",");
    String message_index = message.substring(index + 1, message.length());
    Serial1.println("AT+CMGR=" + String(message_index));
}

void Modem_cmgs(String message){
    if(io_mode == true){
        Serial.println("Called: Modem_cmgs()");
        Serial.println("Send AT-Command: AT+CMGS");
    }
    Serial1.println("AT+CMGS=\"" + sender + "\"");
    delay(100);
    Serial1.println(message + "\x1a");
}

void Mqtt_init_topic(String message){
    if(io_mode == true){
        Serial.println("Called: Mqtt_init_topic()");
    }
    int index1 = message.indexOf("\"") + 1;
    int index2 = message.indexOf("\"", index1);
    modem_number = message.substring(index1, index2);
    sub_topic += modem_number + "\"";
    pub_topic += modem_number + "\"";
    mqtt_ready = true;
}

void Mqtt_open(String ip, String port){
    if(io_mode == true){
        Serial.println("Called: MqttOpen()");
        Serial.println("Send AT-Command: AT+QMTOPEN=0," + ip + "," + port);
    }
    Serial1.println("AT+QMTOPEN=0," + ip + "," + port);
}

void Mqtt_conn(String id, String username, String pass){
    if(io_mode == true){
        Serial.println("Called: MqttConn()");
        Serial.println("Send AT-Command: AT+QMTCONN");
    }
        Serial1.println("AT+QMTCONN=0,\"" + id + "\"," + username + "," + pass);
        //Serial1.println("AT+QMTCONN=0,\"" +  id + "\"");
}

void Pub(String topic, String message){
    if(io_mode == true){
        Serial.println("Called: Pub()");
        Serial.println("Send AT-Command: AT+QMTPUB");
        Serial.println("Send Message: " + message);
    }
    Serial1.println("AT+QMTPUB=0,0,0,0," + topic);
    delay(100);
    Serial1.println(message + "\x1a");
}

void Sub(String topic){
    if (io_mode == true){
        Serial.println("Called: Sub()");
        Serial.println("Send AT-Command: AT+QMTSUB");
    }
    Serial1.println("AT+QMTSUB=0,1," + topic + ",2");
}

void Change_ip(String addr){
    if (io_mode == true){
        Serial.println("Called: Change_ip()");
    }
    addr = "\"" + addr + "\""; 
    for(int index = 0; index < memory_length; index++){
        EEPROM.write(index, 0);
    }
    for(int index = 0; index < addr.length(); index++){
        EEPROM.write(index, addr[index]); 
    }
    Modem_reset();
}

boolean is_memory(){
    if (io_mode == true){
        Serial.println("Called: is_memory()");
    }
    if (EEPROM.read(0) == 34){
        return true;
    }
    return false;
}

void MessageFilter(String message){
    if(message.startsWith("ERROR")){
        Modem_reset();
    }else{
        if(message.startsWith("+QIND:")){
            Modem_init();
            Modem_cnum();
        }else if(message.startsWith("+CNUM:")){
            Mqtt_init_topic(message);
            Mqtt_open(mqttip, mqttport);
        }else if(message.startsWith("+CMTI:")){
            delay(100);
            Modem_cmgr(message);
        }else if(message.startsWith("+CMGR:")){
            int index1 = message.indexOf(",") + 2;
            int index2 = message.indexOf(",", index1) - 1;
            sender = message.substring(index1, index2);
            Modem_sns_memory_reset();
        }else if(message.startsWith("+changeip")){
            int index = message.indexOf(":") + 1;
            String addr = message.substring(index, message.length());
            addr.trim();
            Change_ip(addr);
        }else if(message.startsWith("+status")){
            String info = "status: ok,\nmodem number: " + modem_number + ",\n" + "Mqtt IPaddr: " + mqttip;
            Modem_cmgs(info);
        }
        if(mqtt_ready == true){
            if(message.startsWith("+QMTOPEN:")){
                Mqtt_conn(modem_number, username, password);
            }else if(message.startsWith("+QMTCONN:")){
                Sub(sub_topic);
            }else if(message.startsWith("+QMTSUB:")){
                Pub(pub_topic, "ok");
            }else if(message.startsWith("+QMTRECV")){
                message.trim();
                if(message.endsWith("\"open\"")){
                    digitalWrite(gate_on_pin, HIGH);
                    delay(200);
                    digitalWrite(gate_on_pin, LOW);
                    if(io_mode == true){
                        Serial.println("gate open");      
                    }
                    gate_status_open_lock = false;
                }else if(message.endsWith("\"close\"")){
                    digitalWrite(gate_off_pin, HIGH);
                    delay(200);
                    digitalWrite(gate_off_pin, LOW);
                    if(io_mode == true){
                        Serial.println("gate close");      
                    }
                    gate_status_close_lock = false;
                }
            }
        }
    }
}

void setup(){
    Serial.begin(9600);
    Serial1.begin(115200);
    pinMode(modem_onoff_reset_pin, OUTPUT);
    pinMode(gate_on_pin, OUTPUT);
    pinMode(gate_off_pin, OUTPUT);
    pinMode(infrared_vcc1, OUTPUT);
    pinMode(infrared_vcc2, OUTPUT);
    pinMode(infrared_pin1, INPUT);
    pinMode(infrared_pin2, INPUT);
    digitalWrite(modem_onoff_reset_pin, HIGH);
    digitalWrite(infrared_vcc1, HIGH);
    digitalWrite(infrared_vcc2, HIGH);
    time_lock = true;
    gate_status_open_lock = true;
    gate_status_close_lock = true;
    if(io_mode == true){
        Serial.println("board setup");      
    }
    Modem_reset();
}

void loop(){
    if(time_lock == true){
        pre_time = millis();
        time_lock = false;
    }else if(((millis() - pre_time)/1000)/60 >= pub_time_term && time_lock == false){
        if(io_mode == true){
            Serial.println("time = " + String(millis() / 60000) + " min");
        }
        Pub(pub_topic, "ok");
        time_lock = true;
    }

    if(Serial.available()){
        Serial1.write(Serial.read());
    }
    
    if(Serial1.available()){
        String message = Serial1.readStringUntil('\n');
        if(io_mode == true){
            Serial.println(message);
        }
        MessageFilter(message);
    }

    if(gate_status_open_lock == false){
      int voltage1 = map(analogRead(infrared_pin1), 0, 1023, 0, 5000);
      int voltage2 = map(analogRead(infrared_pin2), 0, 1023, 0, 5000);
//      Serial.print("v1: " + String(voltage1) +  ", ");
//      Serial.println("v2: " + String(voltage2) +  ", ");
      if(voltage1 <= open_threshold && voltage2 <= open_threshold){     //open
          Pub(pub_topic, "ok_opened");
          gate_status_open_lock = true;
      }
      delay(200);
    }else if(gate_status_close_lock == false){
      int voltage1 = map(analogRead(infrared_pin1), 0, 1023, 0, 5000);
      int voltage2 = map(analogRead(infrared_pin2), 0, 1023, 0, 5000);
//      Serial.print("v1: " + String(voltage1) +  ", ");
//      Serial.println("v2: " + String(voltage2) +  ", ");
      if(voltage1 >= close_threshold && voltage2 >= close_threshold){     //close
          Pub(pub_topic, "ok_closed");
          gate_status_close_lock = true;
      }
      delay(200);
    }
}
