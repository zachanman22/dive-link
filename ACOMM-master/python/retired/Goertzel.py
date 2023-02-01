
from math import cos, pi, sin


class Goertzel:
    def __init__(self, target_frequency, sample_frequency, n):
        self.sample_frequency = sample_frequency
        self.target_frequency = target_frequency
        self.n = n
        self.samplesAdded = 0
        omega = (2*pi*self.target_frequency)/self.sample_frequency
        self.cosine = cos(omega)
        self.coeff = 2*self.cosine
        self.sine = sin(omega)
        self.Q2 = 0
        self.Q1 = 0

    def resetGoertzel(self):
        self.Q2 = 0
        self.Q1 = 0
        self.samplesAdded = 0

    def samplesReadyToProcess(self):
        if self.samplesAdded >= self.n:
            return True
        return False
    
    def addSample(self, zero_centered_scaled_sample):
        self.samplesAdded += 1
        Q0 = self.coeff*self.Q1-self.Q2+zero_centered_scaled_sample
        self.Q2 = self.Q1
        self.Q1 = Q0

    def calcMagnitudeSquared(self):
        return self.Q1**2 + self.Q2**2 - self.coeff*self.Q1*self.Q2

    def calcPurity(self):
        return (2*self.calcMagnitudeSquared() / self.n)

    def calcRealPart(self):
        return self.Q1 - self.Q2 * self.cosine

    def calcImagPart(self):
        return self.Q2 * self.sine
    
    def detect(self):
        purity = self.calcPurity()
        self.resetGoertzel()
        return purity

    def purityWithDifferentN(self, newN):
        return (2*self.calcMagnitudeSquared()/newN)

    def detectWithDifferentN(self, newN):
        purity = self.purityWithDifferentN(newN)
        self.resetGoertzel()
        return purity

    def detectBatch(self, samples):
        self.resetGoertzel()
        for x in samples:
            self.addSample(x)
        purity = self.detectWithDifferentN(len(samples))
        return purity