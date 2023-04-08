from numpy import fft
import numpy as np
from matplotlib import pyplot as plt

import pandas as pd

df = pd.read_csv('64_94k____sampleK_250_all_G.txt', sep=' ', header=None, names=['Timestamp', 'Data'])

df['Data'].to_csv("testing.csv")
data = df['Data'].to_numpy()

data_to_fft = data[:]
data_size = data_to_fft.size
freqs = fft.fftfreq(data_size, 1/250000)
data_fft = np.abs(fft.fft(data_to_fft))

# freqs = fft.fftshift(freqs)
# data_fft = fft.fftshift(data_fft)

# freqs = freqs[10:data_size//2]
# data_fft = data_fft[10:data_size//2]

print(data_fft)

print(data_to_fft)

plt.subplot(2,1,1)
plt.plot(data_to_fft)

plt.subplot(2,1,2)
plt.plot(freqs, data_fft)
plt.show()
