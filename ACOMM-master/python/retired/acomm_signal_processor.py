from threading import Thread
import threading
import time
import queue
import serial
from ACOMM import ACOMM

analog_queue = queue.Queue()

def data_acquisition_serial_read():
    global analog_queue
    while True:
        try:
            with serial.Serial("\\\\.\\COM9", 115200, timeout=0) as ser:
                time.sleep(2)
                ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
                time.sleep(1)
                newline = ser.read(ser.in_waiting)
                previousMessage = ''
                while True:
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
                    newline = [x[1:-1].split(" ") for x in newline]
                    analog_queue.put(newline)
        except:
            print("serial port did not open")
            time.sleep(1)

# data deque
def data_deque():
    global analog_queue, acomm
    while True:
        while not analog_queue.empty():
            startTime = time.time()
            analog_reads = analog_queue.get()
            samples = len(analog_reads)
            for single_read in analog_reads:
                acomm.addSample(single_read[1], single_read[0])
                if(acomm.isMessageReady()):
                    acomm.printMessages()
            print("Time to process each sample: ", (time.time()-startTime)*1000000/samples)



if __name__ == '__main__':
    print("In main block")
    filepath = "transducer\\tests\\acoustic_tank\\2022-05-25\\50\\8m\\25_32k__3000gain__10__sampleK_200_507934.txt"
    with open(filepath, 'r') as f:
        data = f.readlines()
    data = [x.split('\n')[0] for x in data]
    data = [x.split(' ') for x in data]
    data = [[int(x[0]),int(x[1])] for x in data]
    analog_queue.put(data)
    acomm = ACOMM(200000,50,32768)
    # t1 = Thread(target=data_acquisition_serial_read)
    # t1.setDaemon(True)
    # threads = [t1]
    t2 = Thread(target=data_deque)
    t2.setDaemon(True)
    # threads += [t2]

    # t1.start()
    t2.start()

    while True:
        time.sleep(1)

    # for tloop in threads:
    #     tloop.join()

    # print("End of main block")