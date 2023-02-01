from reedsolo import RSCodec, ReedSolomonError
from bitstring import BitArray
import random
from Hamming import Hamming
import time
import argparse
import serial

def encodeMessage(message, packet_size):
    hammingList = []
    adjustedMessage = message
    if len(adjustedMessage)%packet_size != 0:
        adjustedMessage = adjustedMessage + "".join(['\0']*(packet_size-len(adjustedMessage)%packet_size))
    numberOfMessageSections = int((len(adjustedMessage))/packet_size)
    for x in range(numberOfMessageSections):
        messageSection = adjustedMessage[x*packet_size:(x+1)*packet_size]
        rsc_encoded_message = rsc.encode(messageSection.encode())
        reed_enc_bit_array = BitArray(bytes=rsc_encoded_message)
        reed_enc_bit_list_ints = [int(x) for x in reed_enc_bit_array.bin]
        hammingList.extend(hamm.encode(reed_enc_bit_list_ints))
    encodedString = "".join([str(x) for x in hammingList])
    return encodedString

def decodeMessage(encodedMessage, packetSize):
    adjustedEncodedMessage = encodedMessage
    bitsPerMessage = int((packetSize+rscodecSize)*8*(hamm.encodedMessageLength/hamm.rawMessageLength))
    if len(adjustedEncodedMessage)%bitsPerMessage != 0:
        adjustedEncodedMessage = adjustedEncodedMessage[:int(len(adjustedEncodedMessage)/bitsPerMessage)*bitsPerMessage]
    numberOfMessageSections = int(len(adjustedEncodedMessage)/bitsPerMessage)
    # print("Number of messages sections:", numberOfMessageSections)
    decodedMessages = []
    for x in range(numberOfMessageSections):
        enc_bit_list_ints = [int(x) for x in adjustedEncodedMessage[x*bitsPerMessage:(x+1)*bitsPerMessage]]
        hammDecodedTuple = hamm.decode(enc_bit_list_ints)
        solomonEncodedBitArray = BitArray(bin="".join([str(x) for x in hammDecodedTuple[0]]))
        try:
            erase_pose = hammDecodedTuple[2][:min(erasures,len(hammDecodedTuple[2]))]
            hammNumber = int(hammingSize/8)
            if hammNumber != 1:
                if hammNumber == 0:
                    erase_pose = list(set([int(x/2) for x in erase_pose]))
                else:
                    erase_pose = [y+x*hammNumber for x in erase_pose for y in range(hammNumber)]

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


def get_selected_settings(noise_setting, packet_size):
    settings = {
        0: (4, 64),
        1: (8, 64),
        2: (8, 48),
        3: (8, 40),
        4: (8, 32),
        5: (8, 28),
        6: (8, 24),
        7: (8, 20),
        8: (8, 16),
        9: (8, 12),
        10: (8, 8),
        11: (8, 4),
        12: (32, 8),
        13: (64, 8),
        14: (32, 4),
    }.get(noise_setting, (8, 24))
    settings = (settings[0], int(settings[1]*(packet_size/16)))
    return settings


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ACOMM encoder')
    parser.add_argument("-s", "--serial_port", type=str, help="serial port", required=True)
    parser.add_argument("-n", "--noise_setting", type=int, default=int(7), help="expected channel noise (0: max noise, 14: min noise): [0 to 14]", required=False)
    parser.add_argument("-p", "--packet_size", type=int, default=int(16), help="Default: 16 bytes (min of 8, max of 128)", required=False)
    args = vars(parser.parse_args())

    assert args['packet_size']%8 == 0, "Packet size needs to be divisible by 8"
    assert args['noise_setting'] >= 0 and args['noise_setting'] <= 14, "Noise setting out of bounds"

    # threshold settings
    noise_setting = min(max(0, args['noise_setting']), 14)
    packet_size = min(max(8, args['packet_size']), 128)
    serial_port = args['serial_port']

    #set serial port. If windows, use com port. If not, use direct url
    if "com" in serial_port.lower():
        serial_port = serial_port.lower().split("com")[1]
        serial_port = "\\\\.\\COM" + str(serial_port)
    

    # set hamming and rscodecsize based on noise and packet size
    hammingSize, rscodecSize = get_selected_settings(noise_setting, packet_size)
    hamm = Hamming(hammingSize)
    rsc = RSCodec(rscodecSize)  # 10 ecc symbols
    erasures = rscodecSize/2

    with serial.Serial("\\\\.\\COM4", 115200, timeout=0) as ser:
        time.sleep(3)
        ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
        time.sleep(0.5)
        while True:
            # read the new line
            newMessageIn = ser.readline().decode()
            try:
                decodedMessage = decodeMessage(newMessageIn, packet_size)
                if decodedMessage is None:
                    continue
                print(decodedMessage)
            except:
                pass
            
    # message = "".join(["0123456701234567"]*10)
    # message = message[:packet_size]
    # print("Message:", message)
    # print("Message Length:", len(message))

    # em = encodeMessage(message, packet_size)
    # print(em)
    # print(decodeMessageBitsPerMessage(em, packet_size))
