import time
import serial
from datetime import datetime
from pathlib import Path

if __name__ == '__main__':
    baud = 200
    sampleKhz = 400
    lower_freq_khz = 60
    upper_freq_khz = 80
    seconds = 8
    # message = '1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001111111111111111000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'
    message = '?'
    comPort = 'COM5'
    location = 'lab'
    note = 'test'

    date = datetime.now().date()

    folderPath = './modem/tests/' + str(location) + '/'+str(date) +'/'+ str(baud) + '/'+ 'm'
    Path(folderPath).mkdir(parents=True,exist_ok=True)
    
    fileName =  str(lower_freq_khz) +'_'+str(upper_freq_khz)+ 'k__' + '__sampleK_' + str(sampleKhz) + '_'  + note #str(round(time.time()))[-6:]

    readMeName = fileName + "_readme.txt"
    fileName = fileName + ".txt"
    try:
        with open(folderPath+'/'+readMeName, 'w') as f:
            f.write('\'empty_line\'\n\'' + str(message) + '\'\n' + str(baud))
    except:
        print("readme not available to overwrite")


    filePath = folderPath + '/' + fileName
    # print("FilePath: ", filePath)
    with serial.Serial("\\\\.\\"+ str(comPort), 115200, timeout=0) as ser:
        time.sleep(3)
        ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
        time.sleep(0.5)
        # serial_buffer = b''
        #improved version
        serial_buffer = bytearray()
        with open(filePath, 'w') as log:
            time.sleep(1)
            print("Serial ready")
            firstTime = time.time()
            x = 0
            previousMessage = ''
            listOfStuff = []
            numberOfMessages = 500000*seconds
            ser.read(ser.in_waiting)
            while x < numberOfMessages:

                if ser.in_waiting > 0:
                    serial_buffer += ser.read(ser.in_waiting)
                if b'\n' in serial_buffer:  # split data line by line and store it in var
                    newline = serial_buffer.split(
                        b'\n')
                    serial_buffer = newline[-1]
                    newline = newline[:-1]
                # newline = ser.read(ser.in_waiting)
                # newline = (newline.decode()).split('\n')
                # newline = [x for x in newline if len(x) > 0]
                # if len(newline) == 0:
                #     continue
                # if newline[0][0] != '?':
                #     newline[0] = previousMessage + newline[0]
                # if(newline[-1][-1] != '$'):
                #     previousMessage = newline.pop()
                # else:
                #     previousMessage = ''
                    listOfStuff.extend(newline)
                    x= x+len(newline)
            # print(listOfStuff)
            

            print("Microseconds per message: ", (time.time()-firstTime)/numberOfMessages*1000000)
            print("Total time: ", time.time()-firstTime)
            listOfStuff = [j.decode() for j in listOfStuff]
            # cut out the amount of the first serial buffer
            # print(listOfStuff[0:10])
            posToCut = 0
            for i in range(20, len(listOfStuff)):
                if int(listOfStuff[i][1:-1].split(' ')[0]) - int(listOfStuff[i-1][1:-1].split(' ')[0]) > 50:
                    posToCut = i
                    print("Cutting at: ", posToCut)
                    break
            listOfStuff = listOfStuff[posToCut+20:]

            # print(listOfStuff[0:10])
            # remove duplicates
            # x = 1
            # numDuplicates = 0
            # while x < len(listOfStuff):
            #     if(listOfStuff[x] == listOfStuff[x-1]):
            #         listOfStuff.pop(x)
            #         numDuplicates += 1
            #     else:
            #         x+=1
            # print("Number of duplicates removed: ", numDuplicates)
            #remove last value
            listOfStuff.pop(-1)
            # save file
            for i in range(len(listOfStuff)):
                log.write(listOfStuff[i][0:-1]+'\n')
            print("Saved to: ", filePath)

            