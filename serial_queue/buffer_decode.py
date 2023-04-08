from numpy import fft
import numpy as np
from matplotlib import pyplot as plt

import pandas as pd

import time
import serial
from datetime import datetime
from pathlib import Path


df = pd.read_csv('60_80k____sampleK_250_all_G.txt', sep=' ', header=None, names=['Timestamp', 'Data'])

df['Data'].to_csv("testing.csv")
data = df['Data'].to_numpy()

# data_to_fft = data[744880:744932]
# data_to_fft = data[744877:744877 + 279583]
# data_to_fft = data[744877+1150:744877+1150+1100]
data_to_fft = data[:]
data_size = data_to_fft.size
freqs = fft.fftfreq(data_size, 1/250000)
data_fft = np.abs(fft.fft(data_to_fft))

# freqs = fft.fftshift(freqs)
# data_fft = fft.fftshift(data_fft)

# freqs = freqs[10:data_size//2]
# data_fft = data_fft[10:data_size//2]

plt.subplot(2,1,1)
plt.plot(data_to_fft)

plt.subplot(2,1,2)
plt.plot(freqs, data_fft)
plt.show()

            

fs = 250000
baud_rate = 200

bit_len = fs / baud_rate

bit_len = 1090

threshold = 50000

# Look for 1 as start bit
one_freq_to_search = 76500
# Assume FFT is equal to bit_len (numpy performs the zero padding automatically)
one_freq_bin_to_search = int(one_freq_to_search * bit_len / fs)

# Look for 0
zero_freq_to_search = 54000
# Assume FFT is equal to bit_len (numpy performs the zero padding automatically)
zero_freq_bin_to_search = int(zero_freq_to_search * bit_len / fs)

plus_bins = 2
minus_bins = 4

data = data_to_fft

def max_in_range(fft_seq, freq_bin, plus_bins, minus_bins):
    return np.max(fft_seq[np.max([freq_bin - minus_bins, 0]):np.min([freq_bin + plus_bins, len(fft_seq)])])

def find_start(data, start_at, freq_bin_to_search, threshold, bit_len, plus_bins, minus_bins):

    fft_start_amp = 0

    data_size = data.size

    step_count = start_at
    while fft_start_amp < threshold and step_count < data_size - int(bit_len):
        search_data = data[step_count:step_count + int(bit_len)]

        win_fft = np.abs(fft.fft(search_data))

        # plt.subplot(2,1,1)
        # plt.plot(search_data)
        # plt.subplot(2,1,2)
        # plt.plot(win_fft)
        # plt.show()
        fft_start_amp = max_in_range(win_fft, freq_bin_to_search, plus_bins, minus_bins)
        # print(fft_start_amp)
        step_count += 5
    return step_count - 5

decode_count = find_start(data, 0, zero_freq_bin_to_search, threshold, bit_len, plus_bins, minus_bins)
print(decode_count)

decision_threshold = 50000

bits = []

while decode_count < data_size - int(bit_len):
    search_data = data[decode_count:decode_count + int(bit_len)]

    win_fft = np.abs(fft.fft(search_data))

    # plt.subplot(2,1,1)
    # plt.plot(search_data)
    # plt.subplot(2,1,2)
    # plt.plot(win_fft)
    # plt.show()
    zero_amp = max_in_range(win_fft, zero_freq_bin_to_search, plus_bins, minus_bins)
    one_amp = max_in_range(win_fft, one_freq_bin_to_search, plus_bins, minus_bins)

    if zero_amp > decision_threshold or one_amp > decision_threshold:
        if one_amp > zero_amp:
            bits.append(1)
        else:
            bits.append(0)
        decode_count += int(bit_len)
    else:
        decode_count = find_start(data, decode_count, zero_freq_bin_to_search, threshold, bit_len, plus_bins, minus_bins)

print(bits)

baud = 200
sampleKhz = 250
lower_freq_khz = 60
upper_freq_khz = 80

comPort = 'COM5'
seconds = 10
date = datetime.now().date()

# print("FilePath: ", filePath)
with serial.Serial("\\\\.\\"+ str(comPort), 115200, timeout=0) as ser:
    time.sleep(3)
    ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
    time.sleep(0.5)
    print(ser.in_waiting)
    ser.read(ser.in_waiting)
    numberOfMessages = 1500000/7.5*seconds
    x = 0
    while x < numberOfMessages:
        data = ser.read(ser.in_waiting)
        print(data)
    # with open(filePath, 'w') as log:
    #     time.sleep(1)
    #     print("Serial ready")
    #     firstTime = time.time()
    #     x = 0
    #     previousMessage = ''
    #     listOfStuff = []
    #     numberOfMessages = 1500000/7.5*seconds
    #     ser.read(ser.in_waiting)
    #     while x < numberOfMessages:
    #         newline = ser.read(ser.in_waiting)
    #         newline = (newline.decode()).split('\n')
    #         newline = [x for x in newline if len(x) > 0]
    #         if len(newline) == 0:
    #             continue
    #         if newline[0][0] != '?':
    #             newline[0] = previousMessage + newline[0]
    #         if(newline[-1][-1] != '$'):
    #             previousMessage = newline.pop()
    #         else:
    #             previousMessage = ''
    #         # print(newline)
    #         listOfStuff.extend(newline)
    #         x= x+len(newline)
    #     print("Microseconds per message: ", (time.time()-firstTime)/numberOfMessages*1000000)
    #     print("Total time: ", time.time()-firstTime)
    #     # cut out the amount of the first serial buffer
    #     posToCut = 0
    #     for i in range(20, len(listOfStuff)):
    #         if int(listOfStuff[i][1:-1].split(' ')[0]) - int(listOfStuff[i-1][1:-1].split(' ')[0]) > 50:
    #             posToCut = i
    #             print("Cutting at: ", posToCut)
    #             break
    #     listOfStuff = listOfStuff[posToCut+20:]

    #     # remove duplicates
    #     x = 1
    #     numDuplicates = 0
    #     while x < len(listOfStuff):
    #         if(listOfStuff[x] == listOfStuff[x-1]):
    #             listOfStuff.pop(x)
    #             numDuplicates += 1
    #         else:
    #             x+=1
    #     print("Number of duplicates removed: ", numDuplicates)

    #     # save file
    #     for i in range(len(listOfStuff)):
    #         log.write(listOfStuff[i][1:-1]+'\n')
    #     print("Saved to: ", filePath)

