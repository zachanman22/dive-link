from pyexpat.errors import messages
from random import sample
from Goertzel import Goertzel
from time import time



class ACOMM:
    def __init__(self, sample_frequency, symbolrate, adc_center):
        self.symbolFreq = [25000, 32000]
        self.numSymbols = len(self.symbolFreq)
        self.plusMinusBins = 2
        self.endMessageValue = 255
        self.endMessageThreshold = 0.1
        self.startMessageThresholds = [0.09, 0.10, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.28, 0.5]
        self.numberOfGThreads = len(self.startMessageThresholds)
        
        self.sample_frequency = sample_frequency
        self.symbolrate = symbolrate
        self.adc_center = adc_center
        self.n = int(sample_frequency/symbolrate)

        self.hzPerBin = int(self.sample_frequency/self.n)

        self.symbol_data = []
        for symbol, x in enumerate(self.symbolFreq):
            symbol_dict = {'freq': x, 'symbol': symbol, 'periodUSeconds': (1/x*1000000)}
            g = []
            for k in range(self.numberOfGThreads):
                g.append([])
                for i in range(-self.plusMinusBins,self.plusMinusBins+1):
                    g[k].append(Goertzel(x + self.hzPerBin*i, self.sample_frequency, self.n))
            symbol_dict['g'] = g
            self.symbol_data.append(symbol_dict)

        self.meanTrackerIndex = [0]*self.numberOfGThreads
        self.meanTrackerSize = 3
        self.meanTracker = [[100 for x in range(self.meanTrackerSize)] for y in range(self.numberOfGThreads)]
        print(self.meanTracker)
        self.messagesActive = [0]*self.numberOfGThreads
        self.symbolsInIndex = [0]*self.numberOfGThreads
        self.sampleNumber = [0]*self.numberOfGThreads
        self.messageReadyToRead = False
        self.messageNumber = 0
        self.slidingDetectionWindowSize = 20
        self.sdwindow = [0]*self.slidingDetectionWindowSize
        self.symbolsIn = [[] for k in range(self.numberOfGThreads)]
        for k in range(self.numberOfGThreads):
            self.symbolsIn.append([])

    def resetMeanTracker(self):
        self.meanTracker = [[100 for x in range(self.meanTrackerSize)] for y in range(self.numberOfGThreads)]

    def resetCounters(self):
        for i in range(self.numberOfGThreads):
            self.sampleNumber[i] = 0
            self.symbolsInIndex[i] = 0
            self.messagesActive[i] = 0
        self.symbolsIn = [[] for k in range(self.numberOfGThreads)]
        self.resetMeanTracker()
        self.messageReadyToRead = False
        self.messageNumber += 1

    def addSampleForActiveMessageSingle(self, normalizedSample, k):
        for i in self.symbol_data:
            for goertzel_bin in i['g'][k]:
                goertzel_bin.addSample(normalizedSample)


    def checkForEndOfMessageSingle(self, k):
        for i in self.meanTracker[k]:
            if i > self.endMessageThreshold:
                return False
        return True

    def checkForAllMessagesInactive(self):
        print(self.messagesActive)
        if 1 in self.messagesActive:
            self.messageReadyToRead = False
            return
        for k in range(self.numberOfGThreads):
            if self.messagesActive[k] == 0:
                self.messagesActive[k] = 3
        self.messageReadyToRead = True

    def getSymbolSingle(self, k):
        weightedFrequency = 0
        totalPurity = 0
        for i in self.symbol_data:
            for plusMinusBinIndex, goertzelBin in enumerate(i['g'][k]):
                purity = goertzelBin.detect()
                totalPurity += purity
                weightedFrequency += (i['freq']+(plusMinusBinIndex-self.plusMinusBins)*self.hzPerBin)*purity
        
        mean = totalPurity/(len(self.symbol_data)*(self.plusMinusBins*2+1))
        self.meanTracker[k][self.meanTrackerIndex[k]] = mean
        self.meanTrackerIndex[k] = (self.meanTrackerIndex[k]+1)%self.meanTrackerSize
        weightedFrequency /= totalPurity
        # if k == 0:
        #     print("Mean: ", mean)
        #     print("Weighted frequency: ", weightedFrequency)

        if self.checkForEndOfMessageSingle(k):
            self.messagesActive[k] = 2
            self.checkForAllMessagesInactive()
            return

        self.symbolsIn[k].append(self.getSymbolFromWeightedFrequency(weightedFrequency))
            

    def getSymbolFromWeightedFrequency(self, weightedfrequency):
        print(weightedfrequency)
        frequencyDist = [abs(x['freq']-weightedfrequency) for x in self.symbol_data]
        symbol = frequencyDist.index(min(frequencyDist))
        return str(symbol)

    def addSampleForMessageStartDetection(self, normalizedSample):
        self.sdwindow.append(normalizedSample)
        if(len(self.sdwindow) > self.slidingDetectionWindowSize):
            self.sdwindow.pop(0)


    def hasMessageStarted(self):
        # hello = time()
        purity = 0
        if len(self.sdwindow) < self.slidingDetectionWindowSize:
            return
        for k in range(self.numberOfGThreads):
            if self.messagesActive[k] == 0:
                purity = self.symbol_data[-1]['g'][k][self.plusMinusBins].detectBatch(self.sdwindow)
                if purity > 0.05:
                    print("Purity for has message started: ", purity)
                break
        # print("Has message started: ", (time()-hello)*1000000)
        if purity > self.startMessageThresholds[0]:
            for k in range(self.numberOfGThreads):
                if self.messagesActive[k] == 0 and purity >= self.startMessageThresholds[k]:
                    self.messagesActive[k] = 1
        

    def addSample(self,sample,time):
        # normalize the sample to between -1 to 1
        normalizedSample = (sample-self.adc_center)/(self.adc_center)
        for k in range(self.numberOfGThreads):
            if self.messagesActive[k] == 1:
                self.addSampleForActiveMessageSingle(normalizedSample, k)
                if self.symbol_data[0]['g'][k][0].samplesReadyToProcess():
                    # if k == 0:
                    #     print("Time: ", time)
                    self.getSymbolSingle(k)
        self.addSampleForMessageStartDetection(normalizedSample)
        self.hasMessageStarted()

    def isMessageReady(self):
        return self.messageReadyToRead
    
    def printMessages(self):
        print("Message number: ", self.messageNumber)
        for k in range(self.numberOfGThreads):
            if self.messagesActive[k] == 2:
                print("")
                messages = "".join(self.symbolsIn[k])
                print(messages)
        self.resetCounters()

            

            



    
