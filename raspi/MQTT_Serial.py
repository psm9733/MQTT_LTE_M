import serial
import re
import sys, errno
from multiprocessing import Process
p = re.compile("\+QMT.*:|ERROR")

class LTE_Modem():

    def __init__(self, host, clientid, username, passwd):
        self.portno = "/dev/ttyUSB"
        self.lte_serial = None
        self.host = host
        self.clientid = clientid
        self.username = username
        self.passwd = passwd
        self.pubTopic = ["\"topic/test\""]
        self.subTopic = ["\"topic/test\""]
        self.keywords = ["+QMTOPEN:", "+QMTSTAT:", "+QMTCONN:", "+QMTPUB:", "+QMTSUB:", "+QMTRECV:", "+QMTDISC:", "ERROR"]
        self._setup()
        
    def _setup(self):
        print("_setup")
        for index in range(0, 10):
            try:
                print("Connecting : " + self.portno + str(index))
                self.lte_serial = serial.Serial(self.portno + str(index), 115200, timeout = 1)
                if self.lte_serial.is_open == True:
                    self.lte_serial.close()   
                self.lte_serial.open()
                if self.lte_serial.is_open == True:
                    self.MQTT_connect(self.host)
                    self.portno = self.portno + str(index)
                    print("Connection success : " + self.portno)
                    break
            except IOError:
                print(self.portno + str(index) + " : No connection -> Changing serial port")
        
    def MQTT_connect(self, host):
        print("Send MQTT_connect Message")
        message = "AT+QMTOPEN=0, " + host + ", 1883\r"
        message = message.encode('utf-8')
        self.lte_serial.write(message)
        
    def MQTT_login(self, clientid, username, passwd):
        print("Send MQTT_login Message")
        message = "AT+QMTCONN=0, \"" + clientid + "\", \"" + username + "\", \"" + passwd + "\"\r"
        message = message.encode('utf-8')
        self.lte_serial.write(message)

    def MQTT_publish(self, topic, message):
        print("Send MQTT_publish Message")
        message = "AT+QMTPUB=0,0,0,0" + topic + "\x1A"
        message = message.encode('utf-8')
        self.lte_serial.write(message)
        
    def MQTT_subscribe(self, topic):
        print("Send MQTT_subscribe Message")
        message = "AT+QMTSUB=0,1," + topic + ",2\r"
        message = message.encode('utf-8')
        self.lte_serial.write(message)
        
    def MQTT_disconnect(self):
        print("Send MQTT_disconnect Message")
        message = "AT+QMTDISC=0\r"
        message = message.encode('utf-8')
        self.lte_serial.write(message)
            
    def run(self):
        while True:
            message = self.lte_serial.read_until("\n").decode('utf-8')
            if len(message) > 0:
                print(message)
                keyword = p.findall(message)
                if len(keyword) > 0:
                    keyword = keyword[0]
                    if keyword == self.keywords[0]:
                        self.MQTT_login(self.clientid, self.username, self.passwd)    
                    elif keyword == self.keywords[2]:
                        if len(self.subTopic) > 0:
                            for index in range(len(self.subTopic)):
                                self.MQTT_subscribe(self.subTopic[index])
                    elif keyword == self.keywords[3]:
                        self.MQTT_publish(self.pubTopic[0], "test")
                    elif keyword == self.keywords[5]:
                        print("Receive")
                    elif keyword == self.keywords[7]:
                        self.MQTT_disconnect()
                        self.MQTT_connect(self.host)
   
    def ___del__(self):
        self.MQTT_disconnect()
        self.lte_serial.close()
        self.lte_serial.__del__()

if __name__ == "__main__":
    lte = LTE_Modem("\"broker.hivemq.com\"", "testid", "hansung", "hansung2019")
    lte.run()