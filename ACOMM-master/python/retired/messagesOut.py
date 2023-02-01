import time
import serial
folderPath = "python\\messagesToSend\\"
fileName = "messagesToSend"

def load_messages(folderPath, fileName):
    with open(folderPath+fileName, "r") as f:
        data = f.read()
        return ([x.encode()+str("\n").encode() for x in data.split("\n")])[:-1]



ser = serial.Serial("\\\\.\\COM24", 115200, timeout=0.5)
print("Connected")
data = load_messages(folderPath, fileName)
time.sleep(1)
ser.write(data[0])
print("Message: " + str(0))
i = 1
numberOfMessagesToSend = 100
while i < numberOfMessagesToSend:
    messageIn = ser.readline()
    if(len(messageIn) != 0):
        if("Message" in messageIn.decode()):
            time.sleep(5)
            ser.write(data[i])
            print("Message: " + str(i))
            i += 1
ser.close()

