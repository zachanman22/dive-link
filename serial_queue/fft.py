from numpy import fft
import numpy as np
from matplotlib import pyplot as plt

import pandas as pd

if __name__ == '__main__':

    df = pd.read_csv("60_80k____sampleK_400_queue_faster.txt", header=None, names=['Data'])

    df['Data'].to_csv("testing.csv")
    data = df['Data'].to_numpy()

    bit = 2
    samplePerBit = 2250
    start = 1866200
    data_to_fft = data[:]
    print(data_to_fft[1:20])
    # data_to_fft = data[1609000:2247000] #<
    #data_to_fft = data[start + (bit -1)*2250:start + bit*samplePerBit] #<
    #data_to_fft = data[1546000:2128000] #G
    #data_to_fft = data[675780:955000] #half
    #data_to_fft = data[868822:1148460] #quarter
    data_size = data_to_fft.size
    freqs = fft.fftfreq(data_size, 1/360000)
    data_fft = np.abs(fft.fft(data_to_fft))

    

    # freqs = fft.fftshift(freqs)
    # data_fft = fft.fftshift(data_fft)

    # freqs = freqs[10:data_size//2]
    # data_fft = data_fft[10:data_size//2]





    data_fft[np.argmax(data_fft)] = 0

    print(data_fft)

    print(data_to_fft)

    plt.subplot(2,1,1)
    plt.plot(data_to_fft)

    plt.subplot(2,1,2)
    plt.plot(freqs, data_fft)
    plt.show()
