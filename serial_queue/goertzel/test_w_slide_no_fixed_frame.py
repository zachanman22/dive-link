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
        # print(results)

        # plt.subplot(2,1,1)
        # plt.plot(search_data)
        # plt.subplot(2,1,2)
        # plt.plot(win_fft)
        # plt.show()

        #freqs is array of examined freqs; mag is array of results for said freqs
        mag = np.array(results)[:,2]
        #find max mag and corresponding freq for each bit
        fft_start_amp = np.max(mag)
        
        # print(fft_start_amp)
        step_count += 100
    return step_count - 100

if __name__ == '__main__':
    # quick test
    fileName = "60_80k____sampleK_450_11000110_no1secDelay"
    df = pd.read_csv(fileName + '.txt', header=None, names=['Data'])

    df['Data'].to_csv("testing.csv")
    data = df['Data'].to_numpy()

    #sample data and scale down
    transmission = data / 1000
    plt.plot(transmission)
    plt.show()
    data_size = transmission.size
    print(data_size)
    print(transmission[0:50])
    # generating test signals
    #ADC Sample Rate
    SAMPLE_RATE = 454000
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
    print(WINDOW_SIZE)
    shift = int(WINDOW_SIZE / 4)
    start_threshold = 800
    end_threshold = 100

    decode_index = 0
    while decode_index < data_size - WINDOW_SIZE:
        start_index = find_start(transmission, decode_index, (70000, 80000), start_threshold, int(WINDOW_SIZE / 4), SAMPLE_RATE)
        if(start_index > data_size - WINDOW_SIZE):
            break
        print(start_index)
        startIndex = 0
        bit = 1
        while bit <= 256 and decode_index < data_size - WINDOW_SIZE:
            #indices
            start = start_index + (bit - 1) * WINDOW_SIZE + shift
            end = start_index + bit * WINDOW_SIZE - shift
            
            #calculate range of data to sample based on selected bit
            # t = np.linspace(0, 1.28, data_size)[start:end]
            
            #mutiply by hamming window
            trans = transmission[start:end] * np.hamming(end - start)
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
            if max < end_threshold:
                decode_index = start_index + (bit + 1) * WINDOW_SIZE
                break
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
            decode_index = start_index + bit * WINDOW_SIZE
    with open('goertzel_results_slide_no_fixed_frame' + fileName + '.txt', 'w') as f:
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
    