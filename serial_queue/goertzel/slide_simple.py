import numpy as np
import pylab
import pandas as pd
from goertzel_func import goertzel
import matplotlib.pyplot as plt


def find_start(data, start_at, freq_range:tuple, threshold, win_len, fs):

    fft_start_amp = 0

    data_size = data.size

    step_count = start_at
    while fft_start_amp < threshold and step_count < data_size - int(win_len):
        search_data = data[step_count:step_count + int(win_len)]

        freqs, results = goertzel(search_data, fs, freq_range)

        #freqs is array of examined freqs; mag is array of results for said freqs
        mag = np.array(results)[:,2]
        # print(freqs)
        #find max mag and corresponding freq for each bit
        fft_start_amp = np.max(mag)
        # print(mag)
        # print(fft_start_amp)
        step_count += 100
    return (step_count - 100, fft_start_amp)

if __name__ == '__main__':
    # quick test
    fileName = "60_80k____sampleK_400_queue_faster"
    df = pd.read_csv(fileName + '.txt', header=None, names=['Data'])

    df['Data'].to_csv("testing.csv")
    data = df['Data'].to_numpy()

    #sample data and scale down
    transmission = data / 1000
    plt.plot(transmission)
    plt.show()
    data_size = transmission.size
    # print(data_size)
    # print(transmission[0:50])
    # generating test signals
    #ADC Sample Rate
    SAMPLE_RATE = 400000
    BITRATE = 200
    #actual sample rate (based on number of sample (data_size) and bit rate (200))
    actualSampleRate = int(data_size/(256/200))
    # print(actualSampleRate)
    #Select bit to examine from 1-256
    # bit = 256
    #array of results
    detection = []
    string = ""
    #window size 
    # calculate samples/bit sample rate (samples/secs) / bitrate (bits/sec) = samples/bit
    WINDOW_SIZE = int(SAMPLE_RATE / BITRATE)
    #window size = 2000
    # print(WINDOW_SIZE)

    decode_index = 0
    while decode_index < data_size - WINDOW_SIZE:
        start_index, mag = find_start(transmission, decode_index, (50000, 60000), 100, int(WINDOW_SIZE / 20), SAMPLE_RATE)
        if(start_index > data_size - WINDOW_SIZE):
            break
        print((start_index,mag))
        decode_index = start_index + 256*WINDOW_SIZE
 
    