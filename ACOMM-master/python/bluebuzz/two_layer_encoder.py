# Initialization
from reedsolo import RSCodec, ReedSolomonError
from bitstring import BitArray
import random
from Hamming import Hamming
import time
import ctypes
import binascii

hammingSize = 8
rscodecSize = 64
rsc = RSCodec(rscodecSize)  # 10 ecc symbols
hamm = Hamming(hammingSize)
erasures = rscodecSize/2 #12
print(rsc.maxerrata(erasures=erasures, verbose=True))

def encodeMessage(message):
    hammingList = []
    adjustedMessage = message
    if len(adjustedMessage)%messageBreakLength != 0:
        adjustedMessage = adjustedMessage + "".join(['\0']*(messageBreakLength-len(adjustedMessage)%messageBreakLength))
    numberOfMessageSections = int((len(adjustedMessage))/messageBreakLength)
    for x in range(numberOfMessageSections):
        messageSection = adjustedMessage[x*messageBreakLength:(x+1)*messageBreakLength]
        rsc_encoded_message = rsc.encode(messageSection.encode())
        reed_enc_bit_array = BitArray(bytes=rsc_encoded_message)
        reed_enc_bit_list_ints = [int(x) for x in reed_enc_bit_array.bin]
        hammingList.extend(hamm.encode(reed_enc_bit_list_ints))
    encodedString = "".join([str(x) for x in hammingList])
    return encodedString


# def decodeMessagePacketSize(encodedMessage, packetSize):

def decodeMessageBitsPerMessage(encodedMessage):
    global hammingSize
    adjustedEncodedMessage = encodedMessage
    bitsPerMessage = int((messageBreakLength+rscodecSize)*8*(hamm.encodedMessageLength/hamm.rawMessageLength))
    if len(adjustedEncodedMessage)%bitsPerMessage != 0:
        adjustedEncodedMessage = adjustedEncodedMessage[:int(len(adjustedEncodedMessage)/bitsPerMessage)*bitsPerMessage]
    numberOfMessageSections = int(len(adjustedEncodedMessage)/bitsPerMessage)
    # print("Number of messages sections:", numberOfMessageSections)
    decodedMessages = []
    for x in range(numberOfMessageSections):
        enc_bit_list_ints = [int(x) for x in adjustedEncodedMessage[x*bitsPerMessage:(x+1)*bitsPerMessage]]
        # print(enc_bit_list_ints)
        # print(len(enc_bit_list_ints))
        hammDecodedTuple = hamm.decode(enc_bit_list_ints)
        # print("Single Error Pos:", hammDecodedTuple[1])
        # print("Double Error Pos:",hammDecodedTuple[2])
        solomonEncodedBitArray = BitArray(bin="".join([str(x) for x in hammDecodedTuple[0]]))
        try:
            erase_pose = hammDecodedTuple[2][:min(erasures,len(hammDecodedTuple[2]))]
            hammNumber = int(hammingSize/8)
            if hammNumber != 1:
                if hammNumber == 0:
                    # print("Erase Pose:",erase_pose)
                    # erase_pose = [int(x/2) for i, x in enumerate(erase_pose) if i+1 != len(erase_pose) and int(x) != int(erase_pose[i+1]/2)]
                    erase_pose = list(set([int(x/2) for x in erase_pose]))
                    # print("Erase Pose edited:",erase_pose)
                else:
                    erase_pose = [y+x*hammNumber for x in erase_pose for y in range(hammNumber)]
                # new_pose = [item for sublist in new_pose for item in sublist]
                # print("New Pose:", erase_pose)

            messageDecoded = rsc.decode(solomonEncodedBitArray.bytes, erase_pos=erase_pose)[0].decode()
            # print(messageDecoded)
            if(messageDecoded is None or len(messageDecoded) < 4):
                return None
            decodedMessages.append(messageDecoded)
        except:
            return None
    decodedMessageString = "".join(decodedMessages)
    # print(decodedMessageString)
    return decodedMessageString
    

def decodeMessage(encodedMessage):
    adjustedEncodedMessage = encodedMessage
    bitsPerMessage = int((messageBreakLength+rscodecSize)*8*(hamm.encodedMessageLength/hamm.rawMessageLength))
    if len(adjustedEncodedMessage)%bitsPerMessage != 0:
        adjustedEncodedMessage = adjustedEncodedMessage[:int(len(adjustedEncodedMessage)/bitsPerMessage)*bitsPerMessage]
    numberOfMessageSections = int(len(adjustedEncodedMessage)/bitsPerMessage)
    # print("Number of messages sections:", numberOfMessageSections)
    decodedMessages = []
    for x in range(numberOfMessageSections):
        enc_bit_list_ints = [int(x) for x in adjustedEncodedMessage[x*bitsPerMessage:(x+1)*bitsPerMessage]]
        # print(enc_bit_list_ints)
        # print(len(enc_bit_list_ints))
        hammDecodedTuple = hamm.decode(enc_bit_list_ints)
        # print("Single Error Pos:", hammDecodedTuple[1])
        # print("Double Error Pos:",hammDecodedTuple[2])
        solomonEncodedBitArray = BitArray(bin="".join([str(x) for x in hammDecodedTuple[0]]))
        try:
            erase_pose = hammDecodedTuple[2][:min(erasures,len(hammDecodedTuple[2]))]
            print("Erase pose:", erase_pose)
            messageDecoded = rsc.decode(solomonEncodedBitArray.bytes, erase_pos=erase_pose)[0].decode()
            # print(messageDecoded)
            if(messageDecoded is None or len(messageDecoded) < 4):
                return None
            decodedMessages.append(messageDecoded)
        except:
            return None
    decodedMessageString = "".join(decodedMessages)
    # print(decodedMessageString)
    return decodedMessageString

def addErrorRateProbability(encodedMessage, successRate=0.95):
    bitList = [str(x) for x in encodedMessage]
    bitErrorPos = []
    for i in range(0,len(bitList)):
        if(random.random() >= successRate):
            bitList[i] = str(abs(int(bitList[i])-1))
            bitErrorPos.append(i)
    for i in range(random.randint(0,50)):
        bitList.append(str(random.randint(0,1)))
    # print("Bit Error Positions:", bitErrorPos)
    newEncodedMessage = "".join(bitList)
    return newEncodedMessage



def convert_float_to_binary_string(value: float):
    if type(value) is not float:
        return None
    binValue = bin(ctypes.c_uint32.from_buffer(ctypes.c_float(value)).value)
    paddingArray = ['0']*32
    paddingArray[len(paddingArray)-len(binValue):] = binValue.split('b')[1]
    return "".join(paddingArray)

def convert_binary_string_to_float(bitstring: str):
    bitstring = int('0b'+"".join(bitstring),2)
    return ctypes.c_float.from_buffer(ctypes.c_uint32(bitstring)).value

# print(convert_binary_string_to_float(convert_float_to_binary_string(20.2)))


# value = float(20.2)
# print(binascii.crc32(b"hello world"))
# print(value)
# binValue = bin(ctypes.c_uint32.from_buffer(ctypes.c_float(value)).value)
# print(binValue.split('b')[1])
# value = ['0']*32
# value[len(value)-len(binValue):] = binValue.split('b')[1]
# value = int('0b'+"".join(value),2)
# asciidata = str(binascii.unhexlify('%x' % value).decode('UTF-8'))
# if len(asciidata) < 4:
#     newascii = ['\0']*4
#     newascii[len(asciidata)-len(newascii):] = asciidata

# binarayascii = bin(int(binascii.hexlify(asciidata.encode('UTF-8')), 16))
# print(ctypes.c_float.from_buffer(ctypes.c_uint32(int(binarayascii, 2))).value)

# print(asciidata)
# bitarr = BitArray(bin=value)
# print(bitarr.bin)
# bitarr.bytes
# print(value)
# print(ctypes.c_float.from_buffer(ctypes.c_uint32(int(value, 2))).value)



message = "".join(["Ox@@@@P@sLsAPELsLCPHsLsAPK@@@@PMLsLCQ|MP@@"]*1)
messageBreakLength = int(48*7/8)
failures = 0
tests = 500
errorProb = 0.97
baud = 160
rawLength = len(message)*8
encodedMessage = encodeMessage(message)
print("Raw message length:", rawLength)
print("Hamming length:", hamm.encodedMessageLength)
print("Encoded message length:", len(encodedMessage))
print("Calc encoded message length:", int((messageBreakLength+rscodecSize)*8*(hamm.encodedMessageLength/hamm.rawMessageLength)))
encodedLength = len(encodedMessage)
timeToSend = len(encodedMessage)/baud
print("Payload Percentage:", (rawLength)/encodedLength)
print("Time To Send:", timeToSend)
for x in range(0, tests):
    # print("Test Number:", x)
    encodedMessage = encodeMessage(message)
    print(encodedMessage)
    encodedMessageWithErrors = addErrorRateProbability(encodedMessage, errorProb)
    # print(encodedMessage)
    # print(decodeMessage(encodedMessage))
    # print(encodedMessageWithErrors)
    if decodeMessageBitsPerMessage(encodedMessageWithErrors) is None:
        failures = failures + 1
print("Success Rate:", (tests-failures)/tests)