import time
import serial
import sys

if __name__ == '__main__':

    comPort = 'COM13'
    with serial.Serial("\\\\.\\"+ str(comPort), 115200) as ser:
        time.sleep(1)
        ser.set_buffer_size(rx_size = 1000000, tx_size = 1000000)
        time.sleep(1)

        filepath = "transducer\\tests\\acoustic_tank\\2022-05-25\\50\\8m\\25_32k__3000gain__10__sampleK_200_507934.txt"
        with open(filepath, 'r') as f:
            data = f.readlines()
        data = [x.split('\n')[0] for x in data]
        data = [x.split(' ') for x in data]
        data = [[int(x[0]),int(x[1])] for x in data]
        print("Starting")
        for sample in data:
            newline = ser.read(ser.in_waiting)
            if(len(newline) > 1):
                print(newline)
            try:
                ser.write((str(sample[1])+'\n').encode())
                time.sleep(0.001)
            except:
                newline = ser.read(ser.in_waiting)
                if(len(newline) > 1):
                    print(newline)
                print("Write Exception")
                break