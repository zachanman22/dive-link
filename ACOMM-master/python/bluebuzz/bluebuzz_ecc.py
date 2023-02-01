from reedsolo import RSCodec, ReedSolomonError
from bitstring import BitArray
import random
from Hamming import Hamming
import time
import ctypes
import binascii
import json


class BlueBuzz_ECC:
    def __init__(self, hammingSize: int = 8, rscodecSize: int = 6, messageBreakLength: int = 32):
        self.hammingSize = hammingSize
        if self.hammingSize != 0:
            self.hamm = Hamming(hammingSize)
            self.erasures = rscodecSize/2  # 12
        else:
            self.erasures = 0
        self.rscodecSize = rscodecSize
        self.rsc = RSCodec(rscodecSize)  # 10 ecc symbols
        self.messageBreakLength = messageBreakLength
        self.messageTrimLength = len(self.encode(b'h'))

    def set_rscoded_size(self, rscodecSize):
        self.rscodecSize = rscodecSize
        self.rsc = RSCodec(rscodecSize)  # 10 ecc symbols
        if self.hammingSize != 0:
            self.erasures = rscodecSize/2
        else:
            self.erasures = 0

    def set_message_break_length(self, messageBreakLength):
        self.messageBreakLength = messageBreakLength
        self.messageTrimLength = len(self.encode(b'h'))

    def encode(self, messageByteArray: bytearray):
        hammingList = []
        adjustedByteArray = messageByteArray
        # print('adjusted byte array', adjustedByteArray)
        # print('len of adjusted byte array', len(adjustedByteArray))
        extraBytes = len(adjustedByteArray) % self.messageBreakLength
        # print("extrabytes", extraBytes)
        # pad out the rest of the message with zeros
        if extraBytes != 0:
            adjustedByteArray = messageByteArray + \
                bytearray([0]*(self.messageBreakLength-extraBytes))
            # print('adjusted byte array', adjustedByteArray)
        numberOfMessageSections = int(
            (len(adjustedByteArray))/self.messageBreakLength)
        for x in range(numberOfMessageSections):
            messageSection = adjustedByteArray[x *
                                               self.messageBreakLength:(x+1)*self.messageBreakLength]
            rsc_encoded_message = self.rsc.encode(
                messageSection)
            reed_enc_bit_array = BitArray(bytes=rsc_encoded_message)
            reed_enc_bit_list_ints = [int(x) for x in reed_enc_bit_array.bin]
            if self.hammingSize == 0:
                hammingList.extend(reed_enc_bit_list_ints)
            else:
                hammingList.extend(self.hamm.encode(reed_enc_bit_list_ints))
        encodedString = "".join([str(x) for x in hammingList])
        return encodedString

    # def encode(self, message):
    #     hammingList = []
    #     adjustedMessage = message
    #     if len(adjustedMessage) % self.messageBreakLength != 0:
    #         adjustedMessage = adjustedMessage + \
    #             "".join(['\0']*(self.messageBreakLength -
    #                     len(adjustedMessage) % self.messageBreakLength))
    #     numberOfMessageSections = int(
    #         (len(adjustedMessage))/self.messageBreakLength)
    #     for x in range(numberOfMessageSections):
    #         messageSection = adjustedMessage[x *
    #                                          self.messageBreakLength:(x+1)*self.messageBreakLength]
    #         rsc_encoded_message = self.rsc.encode(
    #             messageSection.encode())
    #         reed_enc_bit_array = BitArray(bytes=rsc_encoded_message)
    #         reed_enc_bit_list_ints = [int(x) for x in reed_enc_bit_array.bin]
    #         hammingList.extend(self.hamm.encode(reed_enc_bit_list_ints))
    #     encodedString = "".join([str(x) for x in hammingList])
    #     return encodedString

    def decodeJson(self, djson):
        if "m" not in djson.keys():
            return None
        if "d" not in djson.keys():
            return None
        messages = djson["d"]
        removedIncompleteMessages = self.filter_incomplete_messages(messages)
        filteredMessages = self.filter_message_too_small(
            removedIncompleteMessages)
        trimmedMessages = self.trim_messages(filteredMessages)
        uniqueTrimmedMessages = self.get_unique_messages(trimmedMessages)
        if uniqueTrimmedMessages is None or len(uniqueTrimmedMessages) == 0:
            return None
        print("Unique trimmed: ", uniqueTrimmedMessages)
        paddedUniqueTrimmedMessages = self.get_padded_unique_trimmed_messages(
            uniqueTrimmedMessages)
        decodedMessages = self.get_decoded_messages_from_unique_messages(
            paddedUniqueTrimmedMessages)
        print("Decoded messages: ", decodedMessages)
        return decodedMessages

    def decode(self, encodedMessage):
        adjustedEncodedMessage = encodedMessage
        if self.hammingSize != 0:
            bitsPerMessage = int((self.messageBreakLength+self.rscodecSize)
                                 * 8*(self.hamm.encodedMessageLength/self.hamm.rawMessageLength))
        else:
            bitsPerMessage = int(8*(self.messageBreakLength+self.rscodecSize))
        if len(adjustedEncodedMessage) % bitsPerMessage != 0:
            adjustedEncodedMessage = adjustedEncodedMessage[:int(
                len(adjustedEncodedMessage)/bitsPerMessage)*bitsPerMessage]
        numberOfMessageSections = int(
            len(adjustedEncodedMessage)/bitsPerMessage)
        # print("Number of messages sections:", numberOfMessageSections)
        decodedMessages = []
        for x in range(numberOfMessageSections):
            enc_bit_list_ints = [
                int(x) for x in adjustedEncodedMessage[x*bitsPerMessage:(x+1)*bitsPerMessage]]
            # print(enc_bit_list_ints)
            # print(len(enc_bit_list_ints))
            if self.hammingSize == 0:
                solomonEncodedBitArray = BitArray(
                    bin="".join([str(x) for x in enc_bit_list_ints]))
                # print(solomonEncodedBitArray)
            else:
                # print("Single Error Pos:", hammDecodedTuple[1])
                # print("Double Error Pos:",hammDecodedTuple[2])
                hammDecodedTuple = self.hamm.decode(enc_bit_list_ints)
                solomonEncodedBitArray = BitArray(
                    bin="".join([str(x) for x in hammDecodedTuple[0]]))

            try:
                if self.hammingSize != 0:
                    erase_pose = hammDecodedTuple[2][:min(
                        self.erasures, len(hammDecodedTuple[2]))]
                    print("Erase pose:", erase_pose)
                    messageDecoded = self.rsc.decode(
                        solomonEncodedBitArray.bytes, erase_pos=erase_pose)[0]
                else:
                    messageDecoded = self.rsc.decode(
                        solomonEncodedBitArray.bytes)[0]
                # print(messageDecoded)
                if (messageDecoded is None or len(messageDecoded) < 4):
                    return None
                # print(messageDecoded)
                decodedMessages.append(messageDecoded)
            except:
                return None
        decodedMessageString = bytearray()
        for message in decodedMessages:
            decodedMessageString += message
        # print(decodedMessageString)
        return decodedMessageString

    # def decode(self, encodedMessage):
    #     adjustedEncodedMessage = encodedMessage
    #     bitsPerMessage = int((self.messageBreakLength+self.rscodecSize)
    #                          * 8*(self.hamm.encodedMessageLength/self.hamm.rawMessageLength))
    #     if len(adjustedEncodedMessage) % bitsPerMessage != 0:
    #         adjustedEncodedMessage = adjustedEncodedMessage[:int(
    #             len(adjustedEncodedMessage)/bitsPerMessage)*bitsPerMessage]
    #     numberOfMessageSections = int(
    #         len(adjustedEncodedMessage)/bitsPerMessage)
    #     # print("Number of messages sections:", numberOfMessageSections)
    #     decodedMessages = []
    #     for x in range(numberOfMessageSections):
    #         enc_bit_list_ints = [
    #             int(x) for x in adjustedEncodedMessage[x*bitsPerMessage:(x+1)*bitsPerMessage]]
    #         # print(enc_bit_list_ints)
    #         # print(len(enc_bit_list_ints))
    #         hammDecodedTuple = self.hamm.decode(enc_bit_list_ints)
    #         # print("Single Error Pos:", hammDecodedTuple[1])
    #         # print("Double Error Pos:",hammDecodedTuple[2])
    #         solomonEncodedBitArray = BitArray(
    #             bin="".join([str(x) for x in hammDecodedTuple[0]]))
    #         try:
    #             erase_pose = hammDecodedTuple[2][:min(
    #                 self.erasures, len(hammDecodedTuple[2]))]
    #             print("Erase pose:", erase_pose)
    #             messageDecoded = self.rsc.decode(
    #                 solomonEncodedBitArray.bytes, erase_pos=erase_pose)[0].decode()
    #             # print(messageDecoded)
    #             if (messageDecoded is None or len(messageDecoded) < 4):
    #                 return None
    #             decodedMessages.append(messageDecoded)
    #         except:
    #             return None
    #     decodedMessageString = "".join(decodedMessages)
    #     # print(decodedMessageString)
    #     return decodedMessageString

    def filter_incomplete_messages(self, messages):
        messages = [x for x in messages if x is not None and x != "null"]
        return messages

    def filter_message_too_small(self, messages):
        # remove all messages that are too small for ecc
        messages = [x for x in messages
                    if len(x)-1 >= self.messageTrimLength]
        return messages

    def trim_messages(self, messages):
        # remove the first bit from all messages
        first_bit_removed_messages = [x[1:] for x in messages]
        trimmed_messages = [x[:int(len(x)/self.messageTrimLength) *
                              self.messageTrimLength] for x in first_bit_removed_messages]
        return trimmed_messages

    def get_unique_messages(self, messages):
        uniqueMessages = set(messages)
        return list(uniqueMessages)

    def get_padded_unique_trimmed_messages(self, messages):
        messagesPadded = []
        for x in messages:
            messagesPadded.append('1' + x[:-1])
            messagesPadded.append('11' + x[:-2])
            messagesPadded.append('111' + x[:-3])
            messagesPadded.append(x[1:]+'1')
            messagesPadded.append(x[2:]+'11')
            messagesPadded.append(x[3:]+'111')
        messages.extend(messagesPadded)
        return messages

    def get_example_program_message():
        exampleMessage = {"m": 4, "d": ["01101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001101010000000110001100110010100111101000011001100000010011010100001111000110101000011111000111101000000011011101111110101011100101011010101011011101010101101110111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111011100100101011110101100111101101001001000100111100000110000110001111011101001100000001110000111011001000101111101001001110101011011111110010000011110000",
                                        "01101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001101010000000110001100110010100111101000011001100000010011010100001111000110101000011111000111101000000011011101111110101011100101011010101011011101010101101110111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111011100100101011110101100111101101001001000100111100000110000110001111011101001100000001110000111011001000101111101001001110101011011111110010000011110000",
                                        "01101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001101010000000110001100110010100111101000011001100000010011010100001111000110101000011111000111101000000011011101111110101011100101011010101011011101010101101110111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111011100100101011110101100111101101001001000100111100000110000110001111011101001100000001110000111011001000101111101001001110101011011111110010000011110011",
                                        "01101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001101010000000110001100110010100111101000011001100000010011010100001111000110101000011111000111101000000011011101111110101011100101011010101011011101010101101110111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100001000001101001101000101100110010110100011011000000001101100000000110111100110001000001101001101000101100110111100110011101111100000100000110100110000100010011100101110101100101101000010000011010011110011110001101111001100111010101101001000001101001110100111100110111100110011001000011101100001000100111100111100000000000000000000000000000000000000000111110000000000000000011111000000000000000000011111000000000000000011111000000111011100100101011110101100111101101001001000100111100000110000110001111011101001100000001110000111011001000101111101001001110101011011111110010000011110011"]}
        exampleMessageEncoded = json.dumps(exampleMessage)
        exampleMessage = json.loads(exampleMessageEncoded)
        return exampleMessage

    def get_pos_of_last_padding_byte(self, message, paddingChar: bytearray):
        for index in range(len(message)-1, 0, -1):
            if paddingChar[0] != message[index]:
                return index+1
        return 0

    def get_decoded_messages_from_unique_messages(self, unique_trimmed_messages):
        decodedMessages = []
        for x in unique_trimmed_messages:
            decodedMess = self.decode(x)
            if decodedMess != None:
                pos_of_last_padding_byte = self.get_pos_of_last_padding_byte(
                    decodedMess, b'\x00')
                decodedMess = decodedMess[:pos_of_last_padding_byte]
                decodedMessages.append(decodedMess)
        # decodedMessages = list(set(decodedMessages))
        return decodedMessages
