import threading
import queue
import serial
import time
import numpy as np
from datetime import datetime
from pathlib import Path

class SerialCommsThread(threading.Thread):
    def __init__(self, port, messageEndToken=b'\r\n', add_new_line_to_transmit=False, baudrate=115200, received_queue=None, to_transmit_queue=None):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.serial_port = serial.Serial(port, baudrate)
        self.serial_buffer = b''
        self.add_new_line_to_transmit = add_new_line_to_transmit
        self.should_stop = False
        self.messageEndToken = messageEndToken
        self.received_queue = received_queue
        self.to_transmit_queue = to_transmit_queue
        self.RUNNING = 0
        self.INITIALIZED = 1
        self.CRASHED = 2
        self.status = self.INITIALIZED
        print("Connected to serial port ", port)

    def handle_transmit_queue(self):
        # check if there is data in the "to transmit queue" to send to the serial port
        if not self.to_transmit_queue.empty():
            data = self.to_transmit_queue.get()
            if not isinstance(data, (bytes, bytearray)):
                data = data.encode()
            print("Data to transmit on port ", self.port, " : ", data)
            if self.add_new_line_to_transmit:
                data = data + b'\n'
                print("Data to transmit on port with added ",
                      self.port, " : ", data)
            self.serial_port.write(data)

    def handle_receive_serial_port(self):
        # check if there is data in the serial port buffer to read
        if self.serial_port.in_waiting > 0:
            data = self.serial_port.read(
                self.serial_port.in_waiting)
            self.serial_buffer = self.serial_buffer + data
            if self.messageEndToken in self.serial_buffer:  # split data line by line and store it in var
                newline = self.serial_buffer.split(
                        self.messageEndToken)
                self.serial_buffer = newline[-1]
                newline = newline[:-1]
                #add to queue
                self.received_queue.put(newline)

    def restart_serial_port_on_failure(self):
        # close port if it needs to be closed
        try:
            self.serial_port.close()
            time.sleep(0.5)
        except:
            pass
        # restart serial port
        try:
            self.serial_port = serial.Serial(
                self.port, self.baudrate)
            time.sleep(0.5)
            self.serInBuffer = b''
            self.status = self.RUNNING
            print("Serial ", self.port, " running")
        except:
            pass

    def run(self):
        self.status = self.RUNNING
        while not self.should_stop:
            try:
                if self.status == self.RUNNING:
                    try:
                        # check if there is data in the main queue to send to the serial port
                        self.handle_transmit_queue()

                        # check if there is data in the serial port buffer to read
                        self.handle_receive_serial_port()
                    except:
                        self.status = self.CRASHED
                        print("Serial ", self.port, " crashed")
                else:
                    # restart port on crash
                    self.restart_serial_port_on_failure()
            except:
                print("Unknown serial error, retrying in 2 secs")
                time.sleep(2)

    def stop(self):
        self.should_stop = True
        self.join()

if __name__ == "__main__":
    external_serial_thread_port = "COM5"

    # create a queue to hold the serial data
    from_external_data_queue = queue.Queue()
    to_external_data_queue = queue.Queue()
    # create and start the serial reader thread
    # messageEndToken=b'stop\n'
    external_serial_thread = SerialCommsThread(
        external_serial_thread_port, add_new_line_to_transmit=False, baudrate=115200, received_queue=from_external_data_queue, to_transmit_queue = to_external_data_queue)
    external_serial_thread.start()

    # do other stuff while serial data is being read and written on the separate thread

    # write data to the serial port by adding it to the main queue
    # main_data_queue.put(b"Hello world")
    size = 4000000
    count = 0
    list = []
    time.sleep(0.01)
    firstTime = time.time()
    while (count < size):
        data = from_external_data_queue.get()
        list.extend(data)
        count += len(data)
        # print(data)
    elapsed = time.time()-firstTime
    print(list[0:20])
    print("Microseconds per message: ", (elapsed)/size*1000000)
    print("Total time: ", elapsed)
    #decode
    list = [j.decode() for j in list]
    #print file
    baud = "200"
    sampleKhz = "400"
    lower_freq_khz = "60"
    upper_freq_khz = "80"
    seconds = round(elapsed,3)
    # message = '1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001111111111111111000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'
    message = 'mystery'
    comPort = 'COM5'
    location = 'lab'
    note = 'queue'

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

    with open(filePath, 'w') as log:
        # save file
        for i in range(len(list)):
            log.write(str(list[i]) +'\n')
        print("Saved to: ", filePath)

    # # read serial data from the queue
    # while True:
    #     try:
    #         input()
    #     except KeyboardInterrupt:
    #         mcu_serial_thread.stop()
    #         external_serial_thread.stop()
    #         break

    
