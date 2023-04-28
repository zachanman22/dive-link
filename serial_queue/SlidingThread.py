import threading
import queue
import serial
import time
from goertzel_func import goertzel
import numpy as np
from datetime import datetime
from pathlib import Path
from matplotlib import pyplot as plt
import re

class SerialCommsThread(threading.Thread):
    def __init__(self, port, messageEndToken=b'\r\n', add_new_line_to_transmit=False, baudrate=115200, received_queue=None, to_transmit_queue=None):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.serial_port = serial.Serial(port, baudrate)
        # self.serial_buffer = b''
        self.serial_buffer = bytearray()
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

    # def handle_transmit_queue(self):
    #     # check if there is data in the "to transmit queue" to send to the serial port
    #     if not self.to_transmit_queue.empty():
    #         data = self.to_transmit_queue.get()
    #         if not isinstance(data, (bytes, bytearray)):
    #             data = data.encode()
    #         print("Data to transmit on port ", self.port, " : ", data)
    #         if self.add_new_line_to_transmit:
    #             data = data + b'\n'
    #             print("Data to transmit on port with added ",
    #                   self.port, " : ", data)
    #         self.serial_port.write(data)

    def handle_receive_serial_port(self):
        # check if there is data in the serial port buffer to read
        if self.serial_port.in_waiting > 0:
            data = self.serial_port.read(
                self.serial_port.in_waiting)
            self.serial_buffer = self.serial_buffer + data
            # print(self.messageEndToken in self.serial_buffer)
            if self.messageEndToken in self.serial_buffer:  # split data line by line and store it in var
                # print(self.serial_buffer)
                newline = self.serial_buffer.split(self.messageEndToken)
                # newline = self.serial_buffer.splitlines()
                # newline = re.split('\r\n|\r',self.serial_buffer)
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
                        # self.handle_transmit_queue()

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

class SlideWinThread(threading.Thread):
    def __init__(self, received_queue=None, to_transmit_queue=None, fs=400000, bitrate = 200, messageSize = 128):
        super().__init__()
        self.received_queue = received_queue
        self.to_transmit_queue = to_transmit_queue
        #0 = sliding window #1 = goertzel
        self.mode = 1
        #sampling frequency
        self.fs = fs
        #bitrate
        self.bitrate = bitrate
        #prev magnitude (for sliding window)
        self.prev = 100
        self.messageSize = messageSize
        self.RUNNING = 0
        self.INITIALIZED = 1
        self.CRASHED = 2
        self.status = self.INITIALIZED
        self.should_stop = False
        # print("Connected to serial port ", port)
    
    def find_start(self, data, freq_range:tuple, threshold, fs):
        norm_samples = 2 * (data) / (np.max(data)) - 1
        # print(norm_samples)
        freqs, results = goertzel(norm_samples, fs, freq_range)

        #freqs is array of examined freqs; mag is array of results for said freqs
        mag = np.array(results)[:,2]
        # print(freqs)
        #find max mag and corresponding freq for each bit
        fft_start_amp = np.max(mag)
        # print(fft_start_amp)
        #Look from transisition from no data (low mag) to data (high mag)
        if fft_start_amp > threshold:# and self.prev < threshold:
            # print(fft_start_amp)
            #self.prev = fft_start_amp
            return True
        #self.prev = fft_start_amp
        return False
    def goertzel2bits(self, transmission):
        #ADC Sample Rate
        SAMPLE_RATE = self.fs
        BITRATE = self.bitrate
        #Select bit to examine from 1-256
        # bit = 256
        bit = 1
        #array of results
        string = ""
        #window size 
        # calculate samples/bit sample rate (samples/secs) / bitrate (bits/sec) = samples/bit
        WINDOW_SIZE = int(SAMPLE_RATE / BITRATE)
        # print(WINDOW_SIZE)
        shift = int(WINDOW_SIZE / 4)
        # shift = 0
        #plot
        # plt.plot(transmission)
        # plt.show()
        transmission = 2 * (transmission) / (np.max(transmission)) - 1
        while bit <= self.messageSize:
            #indices
            start = (bit - 1) * WINDOW_SIZE + shift
            end = bit * WINDOW_SIZE - 2 * shift
            
            #mutiply by hamming window
            trans = transmission[start:end] * np.hamming(end - start)

            # applying Goertzel on those signals, and plotting results
            freqs, results = goertzel(trans, SAMPLE_RATE, (50000, 60000),  (70000, 80000))
        
            #freqs is array of examined freqs; mag is array of results for said freqs
            mag = np.array(results)[:,2]
            #find max mag and corresponding freq for each bit
            maxIndex = np.argmax(mag)
            max = np.max(mag)
            maxfreq = freqs[maxIndex]
            #determine if bit is 1 or 0
            b = int(maxfreq > 70000)
            string += str(b)
            # #make tuple
            # tup = (bit,b, maxfreq, max)
            # #add tup to list
            # detection.append(tup)
            #iterate bit
            bit += 1
        #add string to transmit queue
        # print(string)
        self.to_transmit_queue.put(string)

    def run(self):
        self.status = self.RUNNING
        while not self.should_stop:
            if self.status == self.RUNNING:
                try:
                    #get start_window amount data points from serial
                    threshold = 10
                    start_window = 100
                    find_hop_length = 50
                    size = int (self.fs // self.bitrate * self.messageSize)
                    count = 0
                    fullData = np.array([])

                    # print("sliding window")
                    while (count < start_window):
                        receive = self.received_queue.get()
                        data = np.array([int(x.decode('utf-8')) for x in receive])
                        fullData = np.concatenate((fullData,data))
                        count += data.size
                    # print(count)

                    start_goertzel_bool = False
                    find_start_counter = 0
                    while not start_goertzel_bool and find_start_counter * find_hop_length + start_window < fullData.size:
                        one_hundred_samps = fullData[find_start_counter * find_hop_length:find_start_counter * find_hop_length + start_window]
                        start_goertzel_bool = self.find_start(one_hundred_samps, (53000,60000), threshold, self.fs)
                        find_start_counter += 1

                    # Get just the data that triggers the threshold
                    # Set the count to the length of this data
                    fullData = fullData[find_start_counter * find_hop_length:]
                    count = fullData.size
                    # print(fullData)
                    # sliding Window
                    if(start_goertzel_bool):
                        # print("starting goertzel")
                        #get the rest of the data frame
                        while (count < size):
                            receive = self.received_queue.get()
                            data = np.array([int(x.decode('utf-8')) for x in receive])
                            fullData = np.concatenate((fullData,data))
                            count += data.size
                        # print(count)
                        #goertzel
                        fullData = fullData
                        self.goertzel2bits(fullData)
                except Exception as e:
                    print(e)
                

    def stop(self):
        self.should_stop = True
        self.join()


if __name__ == "__main__":
    external_serial_thread_port = "COM5"

    # create a queue to hold the serial data
    from_external_data_queue = queue.Queue()
    to_RS_queue = queue.Queue()
    # create and start the serial reader thread
    # messageEndToken=b'stop\n'
    external_serial_thread = SerialCommsThread(
        external_serial_thread_port, add_new_line_to_transmit=False, baudrate=115200, received_queue=from_external_data_queue)
    external_serial_thread.start()

    window_thread = SlideWinThread(
        received_queue=from_external_data_queue, to_transmit_queue = to_RS_queue, fs=400000, bitrate=400, messageSize=64)
    window_thread.start()

    time.sleep(0.01)

    #Make a new thread to handle goertzel algorithm


    # do other stuff while serial data is being read and written on the separate thread

    # write data to the serial port by adding it to the main queue
    # main_data_queue.put(b"Hello world")
    # # read serial data from the queue
    while True:
        try:
            data = to_RS_queue.get()
            print(data)
        except KeyboardInterrupt:
            window_thread.stop()
            external_serial_thread.stop()
            break

    
