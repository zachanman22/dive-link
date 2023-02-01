import serial
import time
import json
from serial_port_lister import serial_ports
import string
import random
from datetime import datetime
import keyboard


def calcTimeOfMessage(numOfBytes: int, baudrate: int):
    return 13*(numOfBytes+2)/baudrate


def updateModem(channel: int = None, volume: float = None, baud: int = None, tx: int = None, rx: int = None, response: bool = False, msg=None):
    messageStr = (json.dumps(getUpdateModemMessage(channel=channel, volume=volume, baud=baud,
                  msg=msg, tx=tx, rx=rx, response=response), separators=(',', ':')) + '\n').encode('UTF-8')
    print(messageStr)
    ser.write(messageStr)
    if(response):
        message = ser.readline().decode('UTF-8')
        while('channel' not in message):
            messageStr = (json.dumps(getUpdateModemMessage(channel=channel, volume=volume, baud=baud,
                          msg=msg, tx=tx, rx=rx, response=response), separators=(',', ':')) + '\n').encode('UTF-8')
            ser.write(messageStr)


def getUpdateModemMessage(channel: int = None, volume: float = None, baud: int = None, tx: int = None, rx: int = None, response: bool = False, msg=None):
    message = {}
    if(channel is not None):
        message["channel"] = channel
    if(volume is not None):
        message["volume"] = volume
    if(baud is not None):
        message["baud"] = baud
    if(tx is not None):
        message["tx"] = tx
    if(rx is not None):
        message["rx"] = rx
    if(msg is not None):
        message["msg"] = msg
    if(response):
        message["response"] = "true"
    return message


now = datetime.now()
current_time = now.strftime("%H-%M-%S")
file1 = open(".\\rx_" + str(current_time)+".txt", "w")
sports = serial_ports()
ser = serial.Serial(sports[-1], 115200, timeout=1)  # open serial port
print(ser.name)         # check which port was really used
updateModem(response=True)
print("Starting")


params = {
    "bytes": [1024, 300, 150, 75],
    "channels": [x for x in range(8)],
    "volume": [x/10 for x in range(10, 0, -1)],
    "baud": [2000, 1000, 500]
}

for volume in params['volume']:
    updateModem(volume=volume)
    for channel in params['channels']:
        for baud in params['baud']:
            updateModem(channel=channel, baud=baud)
            checkVal = False
            while(not checkVal):
                try:  # used try so that if user pressed other than the given key error will not be shown
                    if keyboard.is_pressed('q'):  # if key 'q' is press
                        print("next")
                        checkVal = True
                except:
                    pass
                message = ser.readline()
                print(message)

ser.close()
file1.close()
