import os
import sys
import random
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"  # nopep8
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # nopep8
from bitstring import BitArray
from base64 import decode, encode
from Hamming import Hamming
import pygame
import json
import serial
import time
import ctypes
from reedsolo import RSCodec, ReedSolomonError

messageBreakLength = 200
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

def decodeMessage(encodedMessage):
    global messageBreakLength
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
        print(solomonEncodedBitArray)
        try:
            erase_pose = hammDecodedTuple[2][:min(erasures,len(hammDecodedTuple[2]))]
            print("Erase pose:", erase_pose)
            messageDecoded = rsc.decode(solomonEncodedBitArray.bytes, erase_pos=erase_pose)[0].decode()
            # print(messageDecoded)
            if(messageDecoded is None or len(messageDecoded) < 4):
                return None
            decodedMessages.append(messageDecoded)
        except:
            print("failed")
            return None
    decodedMessageString = "".join(decodedMessages)
    # print(decodedMessageString)
    return decodedMessageString

def convert_float_to_binary_string(value: float):
    if type(value) is not float:
        return None
    binValue = bin(ctypes.c_uint32.from_buffer(ctypes.c_float(value)).value).split('b')[1]
    paddingArray = ['0']*32
    paddingArray[len(paddingArray)-len(binValue):] = binValue
    return "".join(paddingArray)


def convert_binary_string_to_float(bitstring: str):
    bitstring = int('0b'+"".join(bitstring),2)
    return ctypes.c_float.from_buffer(ctypes.c_uint32(bitstring)).value

def convert_float_to_char_safe_binary_string(floatToConv:float):
    binarystring = convert_float_to_binary_string(floatToConv)
    if binarystring == None:
        return None
    binaryStrList = ['01' + binarystring[i:i+6] for i in range(0, len(binarystring), 6)]
    binaryStrList[-1] = '01' +  "".join(['0']*(len(binaryStrList[0])-len(binaryStrList[-1]))) + binaryStrList[-1][2:]
    return "".join(binaryStrList)

def convert_char_safe_binary_string_to_float(binarystr:str):
    # break into 8 bit secionts
    binaryStrList = [binarystr[i:i+8] for i in range(0, len(binarystr), 8)]
    # remove the first 2 buffer bits
    binaryStrList = [x[2:] for x in binaryStrList]
    # remove the uneccessary padding on the last value
    binaryStrList[-1] = binaryStrList[-1][4:]
    binarystring = "".join(binaryStrList)
    return convert_binary_string_to_float(binarystring)



def convert_char_safe_binary_string_to_float_binary_string(binarystr:str):
    binaryStrList = [binarystr[i:i+8] for i in range(0, len(binarystr), 8)]
    binaryStrList = [x[2:] for x in binaryStrList]
    binaryStrList[-1] = binaryStrList[-1][4:]
    return "".join(binaryStrList)




def convert_program_message_to_bitstring(message:dict):
    stringList = []
    for key in message.keys():
        char_safe_binary_str = ""
        if key != 'switch':
            char_safe_binary_str = convert_float_to_char_safe_binary_string(message[key])
        elif message[key] == 0:
            char_safe_binary_str = convert_float_to_char_safe_binary_string(0.0)
        else:
            char_safe_binary_str = convert_float_to_char_safe_binary_string(float(100000.0))
        stringList.append(char_safe_binary_str)
    # print(stringList)
    return "".join(stringList)

def convert_program_bitstring_to_message(message:str, messageKeys:list):
    # break string of bits into 32 bit sections
    bitstringList = [message[i:i+48] for i in range(0, len(message), 48)]
    # create the messageDict, and fill with zeros
    messageDict = {}
    keys = [keys for keys in messageKeys]
    for x in keys:
        messageDict[x] = 0
    # loop through each 32 bitstring and convert to float, add to messageDict
    for x in range(len(bitstringList)):
        messageDict[keys[x]] = convert_char_safe_binary_string_to_float(bitstringList[x])
    return messageDict

def char_safe_binary_string_to_encoded_binary_string(binaryStr:str):
    invertedBinaryStr = "".join([str(abs(int(x))) for x in binaryStr])
    # split string into 8 bit sections
    invertedBitstringList = [str(invertedBinaryStr[i:i+8]) for i in range(0, len(invertedBinaryStr), 8)]
    # print(invertedBitstringList)
    
    # convert 8 bits to char and then to string
    message_string = "".join([str(chr(int(x,2))) for x in invertedBitstringList])
    # print(message_string)
    encoded_bit_str = encodeMessage(message_string)
    return encoded_bit_str

def encoded_binary_str_to_decoded_binary_str(encodedBinaryStr:str):
    # split string into 8 bit sections
    decoded_message = decodeMessage(encodedBinaryStr)
    # print(decoded_message)
    binaryStrList = ['0' + bin(ord(x))[2:] for x in decoded_message]
    binaryStrCharSafe = "".join(binaryStrList)
    return binaryStrCharSafe


    # convert_program_bitstring_to_message(binaryStrList, messageKeys)
    
    # binaryStrList = "".join([bin(int(x))[2:] for x in decoded_message])
    # binaryStr = "".join([str(abs(int(x)-1)) for x in binaryStr])
    # print(binaryStr)

    # convert 8 bits to char and then to string
    # print(decoded_binary_str)

def message_dict_to_encoded_bit_string(message):
    message_bitstring = convert_program_message_to_bitstring(message)
    return char_safe_binary_string_to_encoded_binary_string(message_bitstring)

def encoded_bit_string_to_message_dict(encoded_bit_string, messageKeys):
    message_bitstring = encoded_binary_str_to_decoded_binary_str(encoded_message_bitstring)
    return convert_program_bitstring_to_message(message_bitstring, messageKeys)

if __name__ == "__main__":
    # set the message values
    comPort = 'COM5'
    numberOfMessages = 4
    baudrate = 200
    bitsPerMessage = 1386
    bitsPerMessage += 1
    relayResetTimeMillis = 50
    timeBufferMillis = 200
    timeBetweenMessage = bitsPerMessage/baudrate + 2 * \
        relayResetTimeMillis/1000 + timeBufferMillis/1000
    print("Time to transmit", numberOfMessages, "messages:", timeBetweenMessage*numberOfMessages)
    message = {'kp1': 1.0,
                'kd1': 2.2,
                'ki1': 3.3,
                'kp2': 4.4,
                'kd2': 5.5,
                'ki2': 6.6,
                'switch': 100.0}
                

    
    messageBreakLength = int(48/8*len(message.keys()))

    
    # messageString = "10100111111100011110000111101000000010110100000001011010000000101101000000010110101000010010010000000101101110011011100100110011010011100110111001000001110000101000010010010001010111001001100110100111001101110010011001101001000011011010101000010010010010000110001110011011100100110011010011100110111001000001110000101000010010010010110101001000000010110100000001011010000000101101000000010110101000010010010011010100101001100110100111001101110010011001101001000011011010101000100001011111001100101001101010010101000010010010000000101101000000010110110011010010101010000000100100000110101010000000110000010110000100001011000011001111110010011100011101100010100011111110111111010100010001101100011110111010001010001111011100110111000110010101100010111011110110101010101111001001000110111010011110001101010011011010011010001011011101111101101101110001101001001011011010001001100000100100101111101010100111001101110011100101110100110111100111111000100111110001000000100010011111111100011010100000100111111101110110010111111111100011001001000110011010011010000011111000111001100000001101110100111100001000011001011010010010100001111101111101101101111101110100110101101011100000110101110000011101010110100000111100001011100011000010010101100101110000010001000111011000000010000011100110101001101111101000110001100000001111110000101001100100010000100100110001111010010110110010000101001"
    # print(len(messageString))
    # # encoded_message_bitstring = message_dict_to_encoded_bit_string(message)
    # # decoded_message_dict = encoded_bit_string_to_message_dict(encoded_message_bitstring, message.keys())
    # message_bitstring = encoded_binary_str_to_decoded_binary_str(messageString)
    # print( convert_program_bitstring_to_message(message_bitstring, message.keys()))

    message_encoded = message_dict_to_encoded_bit_string(message)
    print(len(message_encoded))
    print(message_encoded)
    encoded_message_bitstring = (message_encoded+'\n').encode('UTF-8')
    print(encoded_message_bitstring)
    lastMessageSent = time.time()
    ser = None
    with serial.Serial("\\\\.\\"+ str(comPort), 115200, timeout=0.01, write_timeout=0.05) as ser:
        time.sleep(3)
        ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
        print("Serial connected")
        time.sleep(0.5)
        messagesSent = 0
        for x in range(numberOfMessages):
            print("Message ", (x+1), " transmitting")
            ser.write(encoded_message_bitstring)
            time.sleep(timeBetweenMessage)
        
        print("Message complete")
    print("Serial port disconnected")