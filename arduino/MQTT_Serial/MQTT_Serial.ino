const int modem_onoff_reset_pin = 2;
const int gate_on_pin = 3;
const int gate_off_pin = 4;
int pub_time_term = 1 * 1; //(mean of 1 is one min)
unsigned long pre_time;
boolean lock = true;
boolean io_mode = true;
String mqttip = "\"broker.hivemq.com\"";
String mqttport = "1883";
String id = "\"testid\"";
String username = "\"hansung\"";
String password = "\"hansung2019\"";
String sub_topic = "\"command/gate/01012345678\"";
String pub_topic = "\"status/gate/01012345678\"";

void Modem_reset(){
    digitalWrite(modem_onoff_reset_pin, LOW);
    delay(200);
    digitalWrite(modem_onoff_reset_pin, HIGH);
}

void MqttOpen(String ip, String port){
    if(io_mode == true){
        Serial.println("Called: MqttOpen()");
        Serial.println("Send AT-Command: AT+QMTOPEN");
    }
    Serial1.println("AT+QMTOPEN=0," + ip + "," + port);
}

void MqttConn(String id, String username, String pass){
    if(io_mode == true){
        Serial.println("Called: MqttConn()");
        Serial.println("Send AT-Command: AT+QMTCONN");
    }
        Serial1.println("AT+QMTCONN=0," + id + "," + username + "," + pass);
}

void Pub(String topic, String message) {
    if(io_mode == true){
        Serial.println("Called: Pub()");
        Serial.println("Send AT-Command: AT+QMTPUB");
        Serial.println("Send Message: " + message);
    }
    Serial1.println("AT+QMTPUB=0,0,0,0," + topic);
    delay(100);
    Serial1.println(message + "\x1a");
}

void Sub(String topic) {
    if (io_mode == true){
        Serial.println("Called: Sub()");
        Serial.println("Send AT-Command: AT+QMTSUB");
    }
    Serial1.println("AT+QMTSUB=0,1," + topic + ",2");
}

void MessageFilter(String message) {
    if(message.startsWith("ERROR")){
        Modem_reset();
    }else{
        if(message.startsWith("+QMTOPEN:")){
            MqttConn(id, username, password);
        }else if(message.startsWith("+QMTCONN:")){
            Sub(sub_topic);
        }else if(message.startsWith("+QMTSUB:")){
            Pub(pub_topic, "OK");
        }else if(message.startsWith("+QMTRECV")){
            message.trim();
            if(message.endsWith("\"+OPEN\"")){
                digitalWrite(gate_on_pin, HIGH);
                delay(200);
                digitalWrite(gate_on_pin, LOW);
                if(io_mode == true){
                    Serial.println("gate open");      
                }
            }else if(message.endsWith("\"+CLOSE\"")){
                digitalWrite(gate_off_pin, HIGH);
                delay(200);
                digitalWrite(gate_off_pin, LOW);
                if(io_mode == true){
                    Serial.println("gate close");      
                }
            }
        }
    }
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(115200);
    pinMode(modem_onoff_reset_pin, OUTPUT);
    pinMode(gate_on_pin, OUTPUT);
    pinMode(gate_off_pin, OUTPUT);
    digitalWrite(modem_onoff_reset_pin, HIGH);
    if(io_mode == true){
        Serial.println("board setup");      
    }
    Modem_reset();
}

void loop() {
    if (lock == true){
        pre_time = millis();
        lock = false;
    }else if(((millis() - pre_time)/1000)/60 >= pub_time_term && lock == false){
        if(io_mode == true){
            Serial.println("time = " + String(millis() / 60000) + " min");
        }
        //Pub(pub_topic, "OK");
        lock = true;
    }

    if(Serial.available()){
        Serial1.write(Serial.read());
    }
    if(Serial1.available()) {
        String message = Serial1.readStringUntil('\n');
        if(io_mode == true){
            Serial.print(message);
            Serial.print('\n'); 
        }
        if(message.startsWith("+QIND:")){
            MqttOpen(mqttip, mqttport);
        }
        MessageFilter(message);
    }
}
