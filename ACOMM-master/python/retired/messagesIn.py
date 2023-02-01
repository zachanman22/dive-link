import time
import serial




baud = 500
volume = 0.1
dist = 4
masterPath = "python\\messagesIn\\"
folderPath = masterPath + str(baud) + "\\"
fileName = str(baud) + "_" + str(int(time.time()))
print(fileName)



ser = serial.Serial("\\\\.\\COM4", 115200, timeout=0.5)
print("Connected")
with open(folderPath+fileName, "w") as f:
    with open(masterPath + "testInfo", "a") as masterFile:
        masterFile.write("file:" + fileName + " baud:"+str(baud) + " volume:" + volume + " distance:" + dist)
    while True:
        newLine = ser.readline()
        if(len(newLine) != 0):
            message = ((newLine.decode())[:-2]).split(" ")
            message = " ".join([str(message[i]) if i == 0 else "\'" + str(message[i]) + "\'" for i in range(len(message))])
            message = message + '\n'
            print(message)
            f.write(message)
        

ser.close()