import os
import sys
import random
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"  # nopep8
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # nopep8
from bitstring import BitArray
from Hamming import Hamming
import json
import serial
import time
from reedsolo import RSCodec, ReedSolomonError
import ctypes

messageBreakLength = 200
hammingSize = 8
rscodecSize = 64
rsc = RSCodec(rscodecSize)  # 10 ecc symbols
hamm = Hamming(hammingSize)
erasures = rscodecSize/2  # 12
print(rsc.maxerrata(erasures=erasures, verbose=True))


def encodeMessage(message, hamm):
    hammingList = hamm.encode(message)
    encodedString = "".join([str(x) for x in hammingList])
    return encodedString


def convertJSONtoBitList(json):
    if not ("h" in json and "f" in json and "u" in json):
        return None

    # print(json["h"]%256)
    heading = '{0:08b}'.format(json["h"] % 256)
    forward = '{0:04b}'.format(max(0, min(json["f"]+7, 15)))
    up = '{0:04b}'.format(max(0, min(json["u"]+7, 15)))
    bitlist = [int(x)-int('0') for x in list(heading + forward + up)]
    # print(bitlist)
    return bitlist


def convertDecodedBitListToJSON(bitList):
    bitString = convertBitListToBitString(bitList)
    bitString = bitString.decode()
    if (len(bitList) < 16):
        return None
    heading = BitArray(bin="0" + bitString[:8]).int
    # print(heading)
    forward = BitArray(bin="0" + bitString[8:12]).int - 7
    # print(forward)
    up = BitArray(bin="0" + bitString[12:]).int - 7
    # print(up)
    # check if program mode has been initialized
    switch = 0
    if forward == 8 and up == 0:
        switch = 10000
        forward = 0
        up = 0
    decoded_dict = {"h": heading, "f": forward, "u": up, "switch": switch}
    return decoded_dict


def convertBitListToBitString(bitlist, addNewLine=False):
    bitstring = "".join([str(x) for x in bitlist])
    if (addNewLine):
        bitstring += '\n'
    bitstring = bitstring.encode()
    return bitstring


def convertBitStringToBitList(bitString):
    bitString = bitString.decode()
    bitString = (bitString.replace("\n", "")).replace("\r", "")
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


def get_random_heading_dict():
    return {"h": int(random.random()*256), "f": int(round(random.random()*14+0.1)-7), "u": int(round(random.random()*14+0.1)-7)}


def get_unique_messages(messageDict, messageLength):
    # print(messageDict)
    # print(messageDict["d"])
    messageDict["d"] = [str(x[1:messageLength+1])
                        for x in messageDict["d"] if len(x) >= messageLength+1]
    if len(messageDict["d"]) == None:
        return None
    uniqueMessages = set(messageDict["d"])
    return uniqueMessages


def decodeMessage(encodedMessage):
    global programModeMessageBreakLength
    messageBreakLength = programModeMessageBreakLength
    adjustedEncodedMessage = encodedMessage
    bitsPerMessage = int((messageBreakLength+rscodecSize)
                         * 8*(hamm.encodedMessageLength/hamm.rawMessageLength))
    if len(adjustedEncodedMessage) % bitsPerMessage != 0:
        adjustedEncodedMessage = adjustedEncodedMessage[:int(
            len(adjustedEncodedMessage)/bitsPerMessage)*bitsPerMessage]
    numberOfMessageSections = int(len(adjustedEncodedMessage)/bitsPerMessage)
    # print("Number of messages sections:", numberOfMessageSections)
    decodedMessages = []
    for x in range(numberOfMessageSections):
        enc_bit_list_ints = [
            int(x) for x in adjustedEncodedMessage[x*bitsPerMessage:(x+1)*bitsPerMessage]]
        # print(enc_bit_list_ints)
        # print(len(enc_bit_list_ints))
        hammDecodedTuple = hamm.decode(enc_bit_list_ints)
        # print("Single Error Pos:", hammDecodedTuple[1])
        # print("Double Error Pos:",hammDecodedTuple[2])
        solomonEncodedBitArray = BitArray(
            bin="".join([str(x) for x in hammDecodedTuple[0]]))
        print(solomonEncodedBitArray)
        try:
            erase_pose = hammDecodedTuple[2][:min(
                erasures, len(hammDecodedTuple[2]))]
            print("Erase pose:", erase_pose)
            messageDecoded = rsc.decode(
                solomonEncodedBitArray.bytes, erase_pos=erase_pose)[0].decode()
            # print(messageDecoded)
            if (messageDecoded is None or len(messageDecoded) < 4):
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
    binValue = bin(ctypes.c_uint32.from_buffer(
        ctypes.c_float(value)).value).split('b')[1]
    paddingArray = ['0']*32
    paddingArray[len(paddingArray)-len(binValue):] = binValue
    return "".join(paddingArray)


def convert_binary_string_to_float(bitstring: str):
    bitstring = int('0b'+"".join(bitstring), 2)
    return ctypes.c_float.from_buffer(ctypes.c_uint32(bitstring)).value


def convert_float_to_char_safe_binary_string(floatToConv: float):
    binarystring = convert_float_to_binary_string(floatToConv)
    if binarystring == None:
        return None
    binaryStrList = ['01' + binarystring[i:i+6]
                     for i in range(0, len(binarystring), 6)]
    binaryStrList[-1] = '01' + "".join(
        ['0']*(len(binaryStrList[0])-len(binaryStrList[-1]))) + binaryStrList[-1][2:]
    return "".join(binaryStrList)


def convert_char_safe_binary_string_to_float(binarystr: str):
    # break into 8 bit secionts
    binaryStrList = [binarystr[i:i+8] for i in range(0, len(binarystr), 8)]
    # remove the first 2 buffer bits
    binaryStrList = [x[2:] for x in binaryStrList]
    # remove the uneccessary padding on the last value
    binaryStrList[-1] = binaryStrList[-1][4:]
    binarystring = "".join(binaryStrList)
    return convert_binary_string_to_float(binarystring)


def convert_char_safe_binary_string_to_float_binary_string(binarystr: str):
    binaryStrList = [binarystr[i:i+8] for i in range(0, len(binarystr), 8)]
    binaryStrList = [x[2:] for x in binaryStrList]
    binaryStrList[-1] = binaryStrList[-1][4:]
    return "".join(binaryStrList)


def convert_program_message_to_bitstring(message: dict):
    stringList = []
    for key in message.keys():
        char_safe_binary_str = ""
        if key != 'switch':
            char_safe_binary_str = convert_float_to_char_safe_binary_string(
                message[key])
        elif message[key] == 0:
            char_safe_binary_str = convert_float_to_char_safe_binary_string(
                0.0)
        else:
            char_safe_binary_str = convert_float_to_char_safe_binary_string(
                float(100000.0))
        stringList.append(char_safe_binary_str)
    # print(stringList)
    return "".join(stringList)


def convert_program_bitstring_to_message(message: str, messageKeys: list):
    # break string of bits into 32 bit sections
    bitstringList = [message[i:i+48] for i in range(0, len(message), 48)]
    # create the messageDict, and fill with zeros
    messageDict = {}
    keys = [keys for keys in messageKeys]
    for x in keys:
        messageDict[x] = 0
    # loop through each 32 bitstring and convert to float, add to messageDict
    for x in range(len(bitstringList)):
        messageDict[keys[x]] = convert_char_safe_binary_string_to_float(
            bitstringList[x])
    return messageDict


def char_safe_binary_string_to_encoded_binary_string(binaryStr: str):
    invertedBinaryStr = "".join([str(abs(int(x))) for x in binaryStr])
    # split string into 8 bit sections
    invertedBitstringList = [str(invertedBinaryStr[i:i+8])
                             for i in range(0, len(invertedBinaryStr), 8)]
    # print(invertedBitstringList)

    # convert 8 bits to char and then to string
    message_string = "".join([str(chr(int(x, 2)))
                             for x in invertedBitstringList])
    # print(message_string)
    encoded_bit_str = encodeMessage(message_string)
    return encoded_bit_str


def encoded_binary_str_to_decoded_binary_str(encodedBinaryStr: str):
    # split string into 8 bit sections
    decoded_message = decodeMessage(encodedBinaryStr)
    print(decoded_message)
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
    message_bitstring = encoded_binary_str_to_decoded_binary_str(
        encoded_bit_string)
    print(message_bitstring)
    return convert_program_bitstring_to_message(message_bitstring, messageKeys)

# {'h': 123, 'f': 6, 'u': -7}
# 01111011010011101000001110
# {"m":1,"d":["10111101101001110100000111000101000110","10111101101001110100000111000101000110","10111101101001110100000111000101000110","00111101101001110100000111000101000110","00111101101001110100000111000101000110","00111101101001110100000111000101010110","00111101101001110100000111000101010","00111101101001110100000111001101010","00111101101001110100000111001101011"]}

# {'h': 111, 'f': 3, 'u': 1}
# 01101111001101010100000001
# {"m":4,"d":["10110111100110101010000000101000110","10110111100110101010000000101000110","10110111100110101010000000101000100","10110111100110101010000000101000110","10110111100110101010000000100000110","10110111100110101010000000101000100","10110111100110101010000000111000000","00110111100110101010000000101000111","00110111100110101010000000111000110"]}

# {"m":4,"d":["10100111111100011110000111101000000010110100000001011010000000101101000000010110101000010010010000000101101110011011100100110011010011100110111001000001110000101000010010010001010111001001100110100111001101110010011001101001000011011010101000010010010010000110001110011011100100110011010011100110111001000001110000101000010010010010110101001000000010110100000001011010000000101101000000010110101000010010010011010100101001100110100111001101110010011001101001000011011010100000001011010000000101101000000010110100000001011010000000101101000000010111001100000010010111000001100001110001000111000001000001010111101101011010000001001011000110011100000100001011101100001100100010000001011110110111101001110011011111011011110111100101001101111001101101000111101011101100101110101010101001011101001101001011110110110010000001010110111100110110101111111011100000011011110111111010000101011110000011000111100101001110100100111111001000101000111010100100111111111000101100000110101001100111100001111111010011100100001000001111011001111001000110001111011111001000000011100110111101111011110110011000000100000110010001010010011111110001111010110000111110111101011010010000111100100101011000011111101110010101100111111101001010000000110001011110110101101110101011000101001110111001100111001111111111111101001011011010000110110111010000011100010111101101010010110101001010111000101010000000110",
#             "10100111111100011110000111101000000010110100000001011010000000101101000000010110101000010010010000000101101110011011100100110011010011100110111001000001110000101000010010010001010111001001100110100111001101110010011001101001000011011010101000010010010010000110001110011011100100110011010011100110111001000001110000101000010010010010110101001000001010110100000001011010000000101101000000010110101000010010010011010100101001100110100111001101110010011001101001000011011010100000001011010000000101101000000010110100000001011010000000101101000000010111001100000010010111000001100001110001000111000001000001010111101101011010000001001011000110011100000100001011101100001100100010000001011110110111101001110011011111011011110111100001001101111001101101000111101011101100101110101010101001011101001101001011110110110010000001010110111100110110101111111011100000011011110111111010000101011110000011000111100101001110100100111111001000101000111010100100111111111000101100000110101001100111100001111111010011100100001000001111011001111001000110001111011111001000000011100110111101111011110110011000000100000110010001010010011111110001111010110000111110111101011010010000111100100101011000011111101110010101100111111101001010000010110001011110110101101110101011000101001110111001100111001111111111111101001011011010100110110111010000011100010111101101010010110101001010111000101010010000110"]}


def get_decoded_messages_from_unique_messages(unique_messages):
    decodedMessages = []
    for x in unique_messages:
        decodedBitTuple = decodeMessage(x, hamm)
        if (len(decodedBitTuple[2]) == 0):
            decoded_dict = convertDecodedBitListToJSON(decodedBitTuple[0])
            decodedMessages.append(decoded_dict)
    return decodedMessages


def get_unique_decoded_messages(list_of_message_dicts):
    list_of_message_dicts_as_str = [
        json.dumps(x) for x in list_of_message_dicts]
    list_of_unique_message_dicts_as_str = list(
        set(list_of_message_dicts_as_str))
    list_of_unique_message_dicts = [json.loads(
        x) for x in list_of_unique_message_dicts_as_str]
    return list_of_unique_message_dicts


def get_example_message_length(hamm):
    message_rand_dict = get_random_heading_dict()
    bitlist = convertJSONtoBitList(message_rand_dict)
    encodedMessageList = encodeMessage(bitlist, hamm)
    return len(encodedMessageList)


def decode_program_message_from_incoming_json(messageDict, messageLength, messageKeys):
    uniqueMessages = get_unique_messages(messageDict, messageLength)
    if uniqueMessages == None or len(uniqueMessages) == 0:
        return None
    decodedMessages = [encoded_bit_string_to_message_dict(
        x, messageKeys) for x in uniqueMessages]
    decodedMessages = [x for x in decodedMessages if x != None]
    return decodedMessages
    # uniqueMessages = [x.encode('UTF-8') for x in uniqueMessages]
    # decodedMessages = get_decoded_messages_from_unique_messages(uniqueMessages)
    # list_of_unique_decoded_message_dicts = get_unique_decoded_messages(decodedMessages)
    # if len(list_of_unique_decoded_message_dicts) != 1:
    #     return None
    # return list_of_unique_decoded_message_dicts[0]


def decode_heading_message_from_incoming_json(messageDict, messageLength):
    uniqueMessages = get_unique_messages(messageDict, messageLength)
    if uniqueMessages == None or len(uniqueMessages) == 0:
        return None
    uniqueMessages = [x.encode('UTF-8') for x in uniqueMessages]
    decodedMessages = get_decoded_messages_from_unique_messages(uniqueMessages)
    list_of_unique_decoded_message_dicts = get_unique_decoded_messages(
        decodedMessages)
    if len(list_of_unique_decoded_message_dicts) != 1:
        return None
    return list_of_unique_decoded_message_dicts


def get_example_program_message():
    exampleMessage = {"m": 4, "d": ["10100111111100011110000111101000000010110100000001011010000000101101000000010110101000010010010000000101101110011011100100110011010011100110111001000001110000101000010010010001010111001001100110100111001101110010011001101001000011011010101000010010010010000110001110011011100100110011010011100110111001000001110000101000010010010010110101001000000010110100000001011010000000101101000000010110101000010010010011010100101001100110100111001101110010011001101001000011011010101000100001011111001100101001101010010101000010010010000000101101000000010110110011010010101010000000100100000110101010000000110000010110000100001011000011001111110010011100011101100010100011111110111111010100010001101100011110111010001010001111011100110111000110010101100010111011110110101010101111001001000110111010011110001101010011011010011010001011011101111101101101110001101001001011011010001001100000100100101111101010100111001101110011100101110100110111100111111000100111110001000000100010011111111100011010100000100111111101110110010111111111100011001001000110011010011010000011111000111001100000001101110100111100001000011001011010010010100001111101111101101101111101110100110101101011100000110101110000011101010110100000111100001011100011000010010101100101110000010001000111011000000010000011100110101001101111101000110001100000001111110000101001100100010000100100110001111010010110110010000101001",
                                    "10100111111100011110000111101000000010110100000001011010000000101101000000010110101000010010010000000101101110011011100100110011010011100110111001000001110000101000010010010001010111001001100110100111001101110010011001101001000011011010101000010010010010000110001110011011100100110011010011100110111001000001110000101000010010010010110101001000000010110100000001011010000000101101000000010110101000010010010011010100101001100110100111001101110010011001101001000011011010101000100001011111001100101001101010010101000010010010000000101101000000010110110011010010101010000000100100000110101010000000110000010110000100001011000011001111110010011100011101100010100011111110111111010100010001101100011110111010001010001111011100110111000110010101100010111011110110101010101111001001000110111010011110001101010011011010011010001011011101111101101101110001101001001011011010001001100000100100101111101010100111001101110011100101110100110111100111111000100111110001000000100010011111111100011010100000100111111101110110010111111111100011001001000110011010011010000011111000111001100000001101110100111100001000011001011010010010100001111101111101101101111101110100110101101011100000110101110000011101010110100000111100001011100011000010010101100101110000010001000111011000000010000011100110101001101111101000110001100000001111110000101001100100010000100100110001111010010110110010000101001"]}
    exampleMessageEncoded = json.dumps(exampleMessage)
    exampleMessage = json.loads(exampleMessageEncoded)
    return exampleMessage


def transmit_message(messageDict, ser):
    global programMode
    if messageDict['switch'] > 1 and 'h' not in messageDict.keys():
        programMode = False
    else:
        programMode = True
    messageJson = messageDict.dumps(messageDict)
    ser.write(messageJson + '\n')


def get_example_program_dict():
    return {'kp1': 1.0,
            'kd1': 2.2,
            'ki1': 3.3,
            'kp2': 4.4,
            'kd2': 5.5,
            'ki2': 6.6,
            'switch': 0}


programMode = True
ser = None
if __name__ == "__main__":
    headingMessageLength = get_example_message_length(hamm)
    comPort = "COM5"

    programModeMessageLength = 1378
    programModeMessageBreakLength = int(
        48/8*len(get_example_program_dict().keys()))

    # exampleMessage = get_example_program_message()
    with serial.Serial("\\\\.\\" + str(comPort), 115200, timeout=0) as ser:
        time.sleep(2)
        ser.set_buffer_size(rx_size=100000, tx_size=100000)
        print("Serial connected")
        time.sleep(0.5)
        while True:
            time.sleep(0.02)
            try:
                messageIn = [((message.decode('UTF-8')).split('\r')
                              [0]).split('\n')[0] for message in ser.readlines()]
                messageIn = json.loads(messageIn[0])
                # messageIn = exampleMessage
            except:
                continue
                # continue
            if programMode:
                decodedMessage = decode_program_message_from_incoming_json(
                    messageIn, programModeMessageLength, get_example_program_dict().keys())
            else:
                decodedMessage = decode_heading_message_from_incoming_json(
                    messageIn, headingMessageLength)

            if decodedMessage != None and len(decodedMessage) != 0:
                transmit_message(decodedMessage[0], ser)
