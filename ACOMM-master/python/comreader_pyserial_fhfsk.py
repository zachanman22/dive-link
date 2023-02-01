import time
import serial
from datetime import datetime
from pathlib import Path

if __name__ == '__main__':
    baud = 100
    volume = 0.1
    dist = 1 #dist
    gain = 30000
    sampleKhz = 200
    seconds = 10
    # opamp = 'LT6236'
    opamp = 'OP820'
    message = '110110000101110001010001000010000010000110011000101000011101110000010110000000101110000010011101011101011100100010001110011111110000110011011010100011000011010000010001111011011011110111000111111011010000000010100111111000011010001010001100100110110011000011110001011110110110001101110010110000101101110100001111001110000111001011100010100000001001000100000100110110010001011010001100001001000001111'
    comPort = 'COM11'
    location = 'kraken_springs'
    frequency_data = '{{25000, 0.45, 33000, 0.2},{28000, 0.35, 36000, 0.45},{26000, 0.2, 34500, 0.5},{30000, 0.4, 36500, 0.7},{27000, 0.25, 35500, 0.35},{32500, 0.5, 42000, 0.85}}'

    timeseconds = time.time()
    date = datetime.now().date()

    folderPath = './transducer/tests/' + str(location) + '/'+str(date)+'/fhfsk/' + str(baud) + '/'+ str(dist) + 'm' + '/' + str(opamp)
    Path(folderPath).mkdir(parents=True,exist_ok=True)
    
    fileName =  'fhfsk__' + str(gain) + 'gain__' + str(round(volume*100)) + '__sampleK_' + str(sampleKhz) + '_'  + str(round(time.time()))[-6:]

    readMeName = fileName + "_readme.txt"
    fileName = fileName + ".txt"



    filePath = folderPath + '/' + fileName
    # print("FilePath: ", filePath)
    with serial.Serial("\\\\.\\"+ str(comPort), 115200, timeout=0) as ser:
        time.sleep(3)
        ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
        time.sleep(0.5)
        with open(filePath, 'w') as log:
            time.sleep(1)
            print("Serial ready")
            firstTime = time.time()
            x = 0
            previousMessage = ''
            listOfStuff = []
            numberOfMessages = 1500000/7.5*seconds
            ser.read(ser.in_waiting)
            while x < numberOfMessages:
                newline = ser.read(ser.in_waiting)
                newline = (newline.decode()).split('\n')
                newline = [x for x in newline if len(x) > 0]
                if len(newline) == 0:
                    continue
                if newline[0][0] != '?':
                    newline[0] = previousMessage + newline[0]
                if(newline[-1][-1] != '$'):
                    previousMessage = newline.pop()
                else:
                    previousMessage = ''
                # print(newline)
                listOfStuff.extend(newline)
                x= x+len(newline)
            print("Microseconds per message: ", (time.time()-firstTime)/numberOfMessages*1000000)
            print("Total time: ", time.time()-firstTime)
            # cut out the amount of the first serial buffer
            posToCut = 0
            for i in range(20, len(listOfStuff)):
                try:
                    if int(listOfStuff[i][1:-1].split(' ')[0]) - int(listOfStuff[i-1][1:-1].split(' ')[0]) > 50:
                        posToCut = i
                        print("Cutting at: ", posToCut)
                        break
                except:
                    posToCut = i+10
                    break
            listOfStuff = listOfStuff[posToCut+20:]

            # remove duplicates
            x = 1
            numDuplicates = 0
            while x < len(listOfStuff):
                if(listOfStuff[x] == listOfStuff[x-1]):
                    listOfStuff.pop(x)
                    numDuplicates += 1
                else:
                    x+=1
            print("Number of duplicates removed: ", numDuplicates)

            # save file
            for i in range(len(listOfStuff)):
                log.write(listOfStuff[i][1:-1]+'\n')
            print("Saved to: ", filePath)

            try:
                with open(folderPath+'/'+readMeName, 'w') as f:
                    f.write('\'empty_line\'\n\'' + str(message) + '\'\n\'' + str(frequency_data) + '\'\n' + str(baud) + '\n' + str(volume) + '\n' + str(gain) + '\n' + str(timeseconds))
            except:
                print("readme not available to overwrite")

            