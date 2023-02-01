from base64 import encode
import numpy as np

class Hamming:
    def __init__(self, rawMessageLength):
        self.rawMessageLength = rawMessageLength
        self.setUpEncodedMessageLengthAndParityBits()
        self.createGeneratorAndParityCheckMatrix()
        self.setUpErrorDictionary()

    def setUpEncodedMessageLengthAndParityBits(self):
        x = 1
        while x**2 - x - 1 < self.rawMessageLength:
            x += 1
        self.encodedMessageLength = self.rawMessageLength +x+1
        self.parityBits = x+1

    def createGeneratorAndParityCheckMatrix(self):
        self.H = np.zeros((self.parityBits, self.encodedMessageLength))
        self.H = self.H.astype(int)
        self.G = np.zeros((self.rawMessageLength, self.encodedMessageLength))
        self.G = self.G.astype(int)

        numbersToUse = [0]*self.rawMessageLength
        numToSkip = 1
        ind = 0
        currentStep = 1

        while ind < self.rawMessageLength:
            if numToSkip == currentStep:
                numToSkip *= 2
            else:
                numbersToUse[ind] = currentStep
                ind += 1
            currentStep += 1
        
        for i in range(self.rawMessageLength-1, -1, -1):
            for j in range(self.parityBits - 2, -1, -1):
                self.H[self.parityBits-1-j, self.rawMessageLength-1-i] = (numbersToUse[i] & (1 << j)) >> j
        
        #set top of row of 1s
        for i in range(self.encodedMessageLength):
            self.H[0, i] = 1

        for i in range(1, self.parityBits):
            self.H[i, self.rawMessageLength+i] = 1

        for i in range(1, self.parityBits):
            for j in range(0, self.encodedMessageLength):
                if(self.H[i, j] == 1):
                    self.H[0, j] = (self.H[0][j] - 1) * -1
        
        # print("H")
        # print(self.H)
        
        for i in range(0, self.rawMessageLength):
            self.G[i, i] = 1
        
        for i in range(0, self.rawMessageLength):
            for j in range(0, self.parityBits):
                self.G[i, self.rawMessageLength+j] = self.H[j, i]
        
        # print("G")
        # print(self.G)

    def setUpErrorDictionary(self):
        self.errorDictionary = np.zeros((self.encodedMessageLength)).astype(int)
        tempMessage = [x%2 for x in range(0, self.rawMessageLength)]
        
        tempEncodedMessage = self.encode(tempMessage)
        for i in range(0, self.encodedMessageLength):
            tempEncodedMessage[i] = (tempEncodedMessage[i] - 1) * -1
            self.errorDictionary[i] = self.getSyndromeVectorAsInteger(tempEncodedMessage)
            tempEncodedMessage[i] = (tempEncodedMessage[i] - 1) * -1
        # print(self.errorDictionary)

    def getSyndromeVectorAsInteger(self, encodedMessage):
        tempSyndrome = np.zeros((self.parityBits)).astype(int)
        for j in range(0, self.parityBits):
            for i in range(0, self.encodedMessageLength):
                tempSyndrome[j] = tempSyndrome[j] + self.H[j, i]*encodedMessage[i]
            tempSyndrome[j] = tempSyndrome[j]%2
        
        syndromeInt = 0
        for i in range(self.parityBits-1,-1,-1):
            if(tempSyndrome[i] == 1):
                syndromeInt = syndromeInt + 2**(self.parityBits-1-i)
        return int(syndromeInt)

    def encodeSection(self, messageIn):
        messageOut = [0]*self.encodedMessageLength
        for j in range(0, self.encodedMessageLength):
            for i in range(0, self.rawMessageLength):
                messageOut[j] = messageOut[j] + messageIn[i] * self.G[i, j]
            messageOut[j] = messageOut[j] % 2
        return messageOut

    def encode(self, messageIn):
        # messageIn = [1, 0, ..., 0, 1, 1] (list of ints)
        # returns encoded list of ints
        if len(messageIn)%self.rawMessageLength != 0:
            # print("old length:", len(messageIn))
            messageIn.extend([0]*(self.rawMessageLength-len(messageIn)%self.rawMessageLength))
            # print("new length:", len(messageIn))
            # print("modulus of new length:", len(messageIn)%self.rawMessageLength)
        hammingList = []
        for x in range(0, len(messageIn), self.rawMessageLength):
            hammingList.extend(self.encodeSection(messageIn[x:x+self.rawMessageLength]))
        return hammingList

    def errorCorrectSection(self, encodedMessageSection):
        # returns (correctedMessage, posOfFixedBit, double error)
        syndromeInt = self.getSyndromeVectorAsInteger(encodedMessageSection)
        if syndromeInt == 0:
            return (encodedMessageSection, -1, False)
        bitIndex = self.getBitIndexOfError(syndromeInt)
        if bitIndex != -1:
            encodedMessageSection[bitIndex] = (encodedMessageSection[bitIndex] - 1) * -1
            return (encodedMessageSection, bitIndex, False)
        return (encodedMessageSection, -1, True)
    
    def decode(self, encodedMessage):
        decodedMessage = []
        singleErrorPos = []
        doubleErrorPos = []
        for x in range(0, len(encodedMessage), self.encodedMessageLength):
            errorCorrectedSectionTuple = self.errorCorrectSection(encodedMessage[x:x+self.encodedMessageLength])
            decodedMessage.extend(errorCorrectedSectionTuple[0][:self.rawMessageLength])
            if(errorCorrectedSectionTuple[2] is True):
                doubleErrorPos.append(int(x/self.encodedMessageLength))
            else:
                if(errorCorrectedSectionTuple[1] != -1):
                    singleErrorPos.append(errorCorrectedSectionTuple[1]+x)
        return (decodedMessage, singleErrorPos, doubleErrorPos)

        
    def getBitIndexOfError(self, syndromeInt):
        for i in range(0, self.encodedMessageLength):
            if self.errorDictionary[i] == syndromeInt:
                return i
            else:
                if self.errorDictionary[i] == 0:
                    return -1
        return -1
if __name__ == "__main__":
    hamm = Hamming(16)
    rawMessage = [0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0]
    # hello = '110010101'.encode('ascii')
    # print(np.fromstring(hello, dtype=int, sep=''))
    encodedMessage = hamm.encode(rawMessage[:hamm.rawMessageLength])
    print("Encoded Message:", encodedMessage)
    print("Encoded message length:", len(encodedMessage))
    for i in range(0, len(encodedMessage)):
        encodedMessage[i] = (encodedMessage[i] - 1) * -1
        # print(encodedMessage)
        print(hamm.errorCorrectSection(encodedMessage))
    