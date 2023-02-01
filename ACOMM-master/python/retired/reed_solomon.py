# Initialization
from reedsolo import RSCodec, ReedSolomonError
from bitstring import BitArray
import random

rsc = RSCodec(10)  # 10 ecc symbols
print(rsc.maxerrata(verbose=True))

def getEncodedBitArray(message):
    encodedmessage = rsc.encode(message.encode())
    return BitArray(bytes=encodedmessage)
    # return BitArray(bytes=encodedmessage).bin.encode()

def getDecodedString(encodedStringBitArray):
    try:
        return rsc.decode(encodedStringBitArray.bytes)[0].decode()
    except:
        return None

def addErrorRateProbability(encodedStringBitArray, successRate=0.8):
    for i in range(0,len(encodedStringBitArray.bin)):
        if(random.random() >= successRate):
            encodedStringBitArray.overwrite('0b' + str(abs(int(encodedStringBitArray.bin[i])-1)), i)


message = "abcdefghijklmnopqrstuvwxyz"
encodedMessage = getEncodedBitArray(message)
addErrorRateProbability(encodedMessage, 0.99)
print(getDecodedString(encodedMessage))

# c = BitArray(bytes=(encodedmessage))
# error_bin = list(c)
# error_bin = ['1' if x else '0' for x in error_bin ]
# for i in range(0, 3):
#     error_bin[i] = str(abs(int(error_bin[i])-1))
# error_bin = "".join(error_bin)
# print(c.bin)
# rec_message = BitArray(bin=error_bin)
# print(rec_message.bin.encode())
# print(len(rec_message.bin))
# print(rec_message.bytes)
# print(rsc.decode(rec_message.bytes)[0].decode())

# b'hello world'
# message = rsc.decode(b'hXXXo worXd\xed%T\xc4\xfdX\x89\xf3\xa8\xaa')[0]     # 5 errors
# print(message)
# rsc.decode(b'hXXXo worXd\xed%T\xc4\xfdXX\xf3\xa8\xaa')[0]        # 6 errors - fail
# Traceback (most recent call last):
#   ...
# reedsolo.ReedSolomonError: Too many (or few) errors found by Chien Search for the errata locator polynomial!