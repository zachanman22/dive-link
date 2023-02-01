from base64 import decode
from bitstring import BitArray
import random
from Hamming import Hamming
import time
import serial
import json
from bitstring import BitArray

def encodeMessage(message, hamm):
    hammingList = hamm.encode(message)
    encodedString = "".join([str(x) for x in hammingList])
    return encodedString

def convertJSONtoBitList(json):
    if not ("h" in json and "f" in json and "u" in json):
        return None

    print(json["h"]%256)
    heading = '{0:08b}'.format(json["h"]%256)
    forward = '{0:04b}'.format(max(0,min(json["f"]+7,15)))
    up = '{0:04b}'.format(max(0,min(json["u"]+7,15)))
    bitlist = [int(x)-int('0') for x in list(heading + forward + up)]
    print(bitlist)
    return bitlist

def convertDecodedBitListToJSON(bitList):
    bitString = convertBitListToBitString(bitList)
    bitString = bitString.decode()
    if(len(bitList) < 16):
        return None
    heading = BitArray(bin="0" + bitString[:8]).int
    print(heading)
    forward = BitArray(bin="0" + bitString[8:12]).int -7
    print(forward)
    up = BitArray(bin="0" + bitString[12:]).int - 7
    print(up)
    decoded_dict = {"h":heading,"f":forward,"u":up}
    return decoded_dict

def convertBitListToBitString(bitlist, addNewLine=False):
    bitstring = "".join([str(x) for x in bitlist])
    if(addNewLine):
        bitstring += '\n'
    bitstring = bitstring.encode()
    return bitstring

def convertBitStringToBitList(bitString):
    bitString = bitString.decode()
    bitString = (bitString.replace("\n","")).replace("\r","")
    bitlist = [int(x)-int('0') for x in list(bitString)]
    if len(bitlist) < 26:
        return None
    return bitlist[:26]


def decodeMessage(bitString, hamm):
    message = convertBitStringToBitList(bitString)
    if message is None:
        return None
    errorCorrectedMessageTuple = hamm.decode(message)
    return errorCorrectedMessageTuple


if __name__ == "__main__":
    

    hammingSize = 8
    hamm = Hamming(hammingSize)

    # '{"i":0,"h":[0,255],"f":[-7,7],"u":[-7,7]}'
    message = '{"h":1,"f":-3,"u":3}'
    message_json = json.loads(message)
    bitlist = convertJSONtoBitList(message_json)
    encodedMessageList = encodeMessage(bitlist, hamm)
    # print(encodedMessageList)
    # encodedMessageList = str(abs(int(encodedMessageList[0])-int('0')-1)) + str(abs(int(encodedMessageList[1])-int('0')-1)) + encodedMessageList[2:]
    encodedMessageBitString = convertBitListToBitString(encodedMessageList, True)
    print(encodedMessageBitString)

    print("decoding test")
    decodedBitTuple = decodeMessage(encodedMessageBitString, hamm)
    if(len(decodedBitTuple[2]) == 0):
        decoded_dict = convertDecodedBitListToJSON(decodedBitTuple[0])
        print(decoded_dict)


    




    

            