import numpy as np
import pylab
import pandas as pd
from goertzel_func import goertzel
if __name__ == '__main__':
    # quick test
    fileName = "60_80k____sampleK_450_00111100"
    df = pd.read_csv(fileName + '.txt', header=None, names=['Data'])

    df['Data'].to_csv("testing.csv")
    data = df['Data'].to_numpy()

    #sample data and scale down
    transmission = data[2005000:2590000] / 1000
    data_size = transmission.size
    print(data_size)
    print(transmission[0:50])
    # generating test signals
    #ADC Sample Rate
    SAMPLE_RATE = 450000
    BITRATE = 200
    #actual sample rate (based on number of sample (data_size) and bit rate (200))
    actualSampleRate = int(data_size/(256/200))
    # print(actualSampleRate)
    #Select bit to examine from 1-256
    # bit = 256
    #array of results
    detection = []
    bit = 1
    string = ""
    #window size 
    # calculate samples/bit sample rate (samples/secs) / bitrate (bits/sec) = samples/bit
    WINDOW_SIZE = int(SAMPLE_RATE / BITRATE)
    print(WINDOW_SIZE)
    shift = -10
    startIndex = 0
    while bit <= 256:
        #indices
        start = (bit - 1) * WINDOW_SIZE 
        end = bit * WINDOW_SIZE
        
        #calculate range of data to sample based on selected bit
        t = np.linspace(0, 1.28, data_size)[start:end]
        
        #mutiply by hamming window
        trans = transmission[start:end] * np.hamming(WINDOW_SIZE)
        # print(trans.size)

        # applying Goertzel on those signals, and plotting results
        freqs, results = goertzel(trans, SAMPLE_RATE, (50000, 60000),  (70000, 80000))
        
        # print(freqs)
        # print(results)
        
        #Plotting
        """
        pylab.subplot(2, 2, 1)
        pylab.title('Transmission of 32 bits, half 0 half 1, bit {}  0: 60kHz 1: 80kHz'.format(bit))
        pylab.plot(t, trans)

        pylab.subplot(2, 2, 3)
        pylab.title('Goertzel Algo, freqency ranges : [50000, 60000] and [70000, 80000]')
        pylab.plot(freqs, np.array(results)[:,2], 'o')
        pylab.ylim([0,100000])
        pylab.show()
        """
        #freqs is array of examined freqs; mag is array of results for said freqs
        mag = np.array(results)[:,2]
        #find max mag and corresponding freq for each bit
        maxIndex = np.argmax(mag)
        max = np.max(mag)
        maxfreq = freqs[maxIndex]
        #determine if bit is 1 or 0
        b = int(maxfreq > 70000)
        string += str(b)
        #make tuple
        tup = (bit,b, maxfreq, max)
        #add tup to list
        detection.append(tup)
        #iterate bit
        bit += 1
        # print(detection)
        #print(string)
        startIndex += shift
    with open('goertzel_results_' + fileName + '.txt', 'w') as f:
        f.write("bit, value, max detected freq, mag\n")
        for tup in detection:
            f.write(','.join(map(str, tup)))
            f.write('\n')
        f.write("Recorded Data\n")
        f.write(string)
        f.close()
    # # generating test signals
    # SAMPLE_RATE = 250000
    # WINDOW_SIZE = 1024
    # t = np.linspace(0, 1, SAMPLE_RATE)[:WINDOW_SIZE]
    # sine_wave = np.sin(2*np.pi*60000*t) + np.sin(2*np.pi*80000*t)
    # sine_wave = sine_wave * np.hamming(WINDOW_SIZE)
    # sine_wave2 = np.sin(2*np.pi*40000*t) + np.sin(2*np.pi*70000*t)
    # sine_wave2 = sine_wave2 * np.hamming(WINDOW_SIZE)

    # # applying Goertzel on those signals, and plotting results
    # freqs, results = goertzel(sine_wave, SAMPLE_RATE, (55000, 65000),  (75000, 85000))

    # pylab.subplot(2, 2, 1)
    # pylab.title('(1) Sine wave 60kHz + 80kHz')
    # pylab.plot(t, sine_wave)

    # pylab.subplot(2, 2, 3)
    # pylab.title('(1) Goertzel Algo, freqency ranges : [55000, 65000] and [75000, 85000]')
    # pylab.plot(freqs, np.array(results)[:,2], 'o')
    # pylab.ylim([0,100000])
    
    # freqs, results = goertzel(sine_wave2, SAMPLE_RATE, (55000, 65000),  (75000, 85000))

    # pylab.subplot(2, 2, 2)
    # pylab.title('(2) Sine wave 40kHz + 70kHz')
    # pylab.plot(t, sine_wave2)

    # pylab.subplot(2, 2, 4)
    # pylab.title('(2) Goertzel Algo, freqency ranges : [400, 500] and [1000, 1100]')
    # pylab.plot(freqs, np.array(results)[:,2], 'o')
    # pylab.ylim([0,100000])

    # pylab.show()
    