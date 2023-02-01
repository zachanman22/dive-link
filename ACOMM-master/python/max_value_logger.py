import time
import serial
from datetime import datetime
from pathlib import Path

if __name__ == '__main__':
    comPort = 'COM18'
    with serial.Serial("\\\\.\\"+ str(comPort), 115200, timeout=0) as ser:
        time.sleep(3)
        ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
        time.sleep(0.5)
        print("Serial ready")
        firstTime = time.time()
        runningMax = 0
        ser.read(ser.in_waiting)
        while True:
            newline = ser.read(ser.in_waiting)
            try: 
                newline = (newline.decode()).split('\n')
                newline = [abs(int(x.split(' ')[1].split('$')[0])-2048) for x in newline if '$' in x and '?' in x]
                if len(newline) == 0:
                    continue
                newestMax = max(newline)
                if(newestMax > runningMax):
                    runningMax = newestMax
                    if(runningMax > 2049):
                        runningMax = 0
                    print(runningMax)
            except:
                pass

            
        