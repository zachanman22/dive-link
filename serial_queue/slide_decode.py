from numpy import fft
import numpy as np
from matplotlib import pyplot as plt

import pandas as pd


df = pd.read_csv('60_80k____sampleK_450_11000110_no1secDelay.txt', sep=' ', header=None, names=['Data'])

df['Data'].to_csv("testing.csv")
data = df['Data'].to_numpy()

fs = 450000
baud_rate = 200

bit_len = fs / baud_rate
print(bit_len)

data_to_fft = data[266250:266250+int(bit_len)]
data_to_fft = data[:]
data_size = data_to_fft.size
freqs = fft.fftfreq(data_size, 1/fs)
data_fft = np.abs(fft.fft(data_to_fft/1000))

# freqs = fft.fftshift(freqs)
# data_fft = fft.fftshift(data_fft)

# freqs = freqs[10:data_size//2]
# data_fft = data_fft[10:data_size//2]

plt.subplot(2,1,1)
plt.plot(data_to_fft)

plt.subplot(2,1,2)
plt.plot(freqs, data_fft)
plt.show()


# bit_len = 1090

threshold = 10000

# Look for 1 as start bit
one_freq_to_search = 76500
# Assume FFT is equal to bit_len (numpy performs the zero padding automatically)
one_freq_bin_to_search = int(one_freq_to_search * bit_len / fs)

# Look for 0
zero_freq_to_search = 54000
# Assume FFT is equal to bit_len (numpy performs the zero padding automatically)
zero_freq_bin_to_search = int(zero_freq_to_search * bit_len / fs)

plus_bins = 2
minus_bins = 10

data = data_to_fft

start_bin_to_search = one_freq_bin_to_search

def max_in_range(fft_seq, freq_bin, plus_bins, minus_bins):
    return np.max(fft_seq[np.max([freq_bin - minus_bins, 0]):np.min([freq_bin + plus_bins, len(fft_seq)])])

def find_start(data, start_at, freq_bin_to_search, threshold, bit_len, plus_bins, minus_bins):

    fft_start_amp = 0

    data_size = data.size

    step_count = start_at
    while fft_start_amp < threshold and step_count < data_size - int(bit_len):
        search_data = data[step_count:step_count + int(bit_len//5)]

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

decode_count = find_start(data, 0, start_bin_to_search, threshold, bit_len, plus_bins, minus_bins)
print(decode_count)

decision_threshold = 10000

bits = []

while decode_count < data_size - int(bit_len):
    search_data = data[decode_count:decode_count + int(bit_len)]
    # search_data = search_data[int(bit_len//4):int(3*bit_len//4)]

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
        decode_count = find_start(data, decode_count+int(bit_len), start_bin_to_search, threshold, bit_len, plus_bins, minus_bins)

print(bits)
print(len(bits))

