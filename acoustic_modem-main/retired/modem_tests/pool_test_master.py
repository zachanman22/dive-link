import serial
import time
import json
from serial_port_lister import serial_ports
import string
import random
from datetime import datetime
import pyperclip as pc


def calcTimeOfMessage(numOfBytes: int, baudrate: int):
    return 13*(numOfBytes+2)/baudrate


def updateModem(channel: int = None, volume: float = None, baud: int = None, tx: int = None, rx: int = None, response: bool = False, debug: bool = False, msg=None):
    messageStr = (json.dumps(getUpdateModemMessage(channel=channel, volume=volume, baud=baud,
                  msg=msg, tx=tx, rx=rx, response=response, debug=debug), separators=(',', ':')) + '\n').encode('UTF-8')
    ser.write(messageStr)
    if(response):
        message = ser.readline().decode('UTF-8')
        while('channel' not in message):
            messageStr = (json.dumps(getUpdateModemMessage(channel=channel, volume=volume, baud=baud,
                          msg=msg, tx=tx, rx=rx, response=response, debug=debug), separators=(',', ':')) + '\n').encode('UTF-8')
            ser.write(messageStr)


def getUpdateModemMessage(channel: int = None, volume: float = None, baud: int = None, tx: int = None, rx: int = None, response: bool = False, debug: bool = False, msg=None):
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
    if(debug is not None):
        message["debug"] = True
    if(response):
        message["response"] = "true"
    return message


def getCurrentModemStatus():
    currentModemStateMessage = (json.dumps(getUpdateModemMessage(response=True),
                                           separators=(',', ':'))+"\n").encode('UTF-8')
    ser.write(currentModemStateMessage)
    channelStateString = ser.readline().decode('UTF-8')
    while('channel' not in channelStateString):
        ser.write(currentModemStateMessage)
        channelStateString = ser.readline().decode('UTF-8')
    return json.loads(channelStateString)


def waitForUserResponse():
    updateModem(response=True)
    return input("Confirm reciever matching, then press enter...")


def sendMessage(numBytes: int, volume: float, baud: int, channel: int, distanceMeters: float):
    global messageNum, current_time, bytesSentPerCycle, bytesSentThisCycle
    message = {"num": messageNum,
               "volume": round(volume, 3),
               "baud": baud,
               "channel": channel,
               "meters": round(distanceMeters, 3),
               "tag": str(current_time)}
    messageString = json.dumps(message, separators=(',', ':')) + ",\"rand\":"
    randomString = ''.join(random.choice(letters)
                           for i in range(numBytes-len(messageString)-1-4))
    message["rand"] = randomString
    messageString = json.dumps(message, separators=(',', ':'))+"\n\n\n"

    print("MsgNum: " + str(messageNum) + ", Bytes: " + str(len(messageString)
                                                           ) + ", Percent: " + ("{:.2f}".format(100*bytesSentThisCycle/bytesSentPerCycle)) + "%")
    bytesSentThisCycle += len(messageString)
    file1.write(messageString)
    messageNum += 1
    updateModem(msg=messageString)


now = datetime.now()
current_time = now.strftime("%d_%m_%y_%H-%M-%S")
file1 = open(".\\tx_" + str(current_time)+".txt", "w")
sports = serial_ports()
print(sports)
ser = serial.Serial(sports[-1], 115200, timeout=0.5)  # open serial port
print(ser.name)         # check which port was really used
messageNum = 0
letters = string.ascii_lowercase + string.ascii_uppercase

params = {
    "bytes": [150, 300, 600, 1024],
    "channels": [x for x in range(5)],
    "volume": [x/10 for x in range(10, 0, -1)],
    "baud": [2000, 1000, 500]
}

distanceMeters = 1.0
messagesPerParameter = 1
numberOfMessagesPerCycle = len(params['bytes'])*messagesPerParameter
volume = 1.0
bytesSentPerCycle = sum(params['bytes'])*messagesPerParameter

print("Est min per distance: " +
      str(sum(params['bytes'])*messagesPerParameter/900*len(params['channels'])))
bytesSentThisCycle = 0
updateModem(volume=volume, response=True)
while True:
    distanceMeters = float(
        input("Type distance (meters) between transducers:  "))
    if(distanceMeters < 0):
        break
    for channel in params['channels']:
        for baud in params['baud']:
            print("\nNext:")
            updateModem(channel=channel, baud=baud)
            print("copy below to reciever (copied to clipboard)")
            messageToCopyIntoReceiver = "{\"channel\":"+str(
                channel)+",\"baud\":" + str(baud)+",\"response\":\"true\"}"
            print(messageToCopyIntoReceiver)
            pc.copy(messageToCopyIntoReceiver)
            print("From clipboard: " + pc.paste())
            waitForUserResponse()
            bytesSentThisCycle = 0
            for numBytes in params['bytes']:
                for x in range(messagesPerParameter):
                    sendMessage(numBytes, volume, baud,
                                channel, distanceMeters)
                    time.sleep(calcTimeOfMessage(numBytes, baud)+1)
                    while ser.in_waiting:  # Or: while ser.inWaiting():
                        try:
                            file1.write(ser.readline().decode('UTF-8'))
                        except:
                            print("write file failure")
                    file1.write(";")

ser.close()
file1.close()

# {"baud":1000,"channel":0,"response":true}
