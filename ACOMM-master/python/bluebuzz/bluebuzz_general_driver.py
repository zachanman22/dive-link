from base64 import encode
import time
import serial
import json
from bluebuzz_ecc import BlueBuzz_ECC


def publishReceivedMessageToExternalComputer(ser, message, bb_ecc: BlueBuzz_ECC):
    decodedMessages = bb_ecc.decodeJson(message)
    if decodedMessages is None or len(decodedMessages) == 0:
        return
    # decodedMessages.sort(key=len, reverse=True)
    messageToPublish = (decodedMessages[0])
    ser.write(messageToPublish)
    ser.flush()


def transmitMessageFromExternalComputer(ser, message, bb_ecc: BlueBuzz_ECC):
    encodedMessage = bb_ecc.encode(message)
    encodedMessageBstring = (encodedMessage + "\n").encode()
    print(encodedMessageBstring)
    ser.write(encodedMessageBstring)


def readNewMessagesFromACOMM(serExternal, serMCU, bb_ecc: BlueBuzz_ECC):
    # read the incoming messages
    messagesReceived = serMCU.readlines()
    for x in messagesReceived:
        try:
            print("Received: ", x)
            publishReceivedMessageToExternalComputer(
                serExternal, json.loads(x), bb_ecc)
        except:
            pass


def readNewMessageFromExternalComputer(serExternal, serMCU, bb_ecc: BlueBuzz_ECC):
    # read the incoming messages
    messagesReceived = serExternal.readlines()
    for x in messagesReceived:
        try:
            print("To transmit: ", x)
            transmitMessageFromExternalComputer(
                serMCU, x, bb_ecc)
        except:
            pass
#     for index in range(len(messagesReceived)):
#         messagesReceived[index] = messagesReceived[index].decode(
#             'UTF-8')
#         activeMessage += messagesReceived[index]
#         if '\n' in messagesReceived[index]:
#             publishReceivedMessageToExternalComputer(
#                 ser, activeMessage)
#             activeMessage = ""
# except:
#     return


def correctComPortForWindows(comport):
    if "COM" in comport:
        return "\\\\.\\" + comport
    return comport


if __name__ == "__main__":
    externalComPort = "/dev/ttyUSB0"
    mcuComPort = "/dev/ttyACM0"
    bb_ecc = BlueBuzz_ECC(0, 10)

    externalComPort = correctComPortForWindows(externalComPort)
    mcuComPort = correctComPortForWindows(mcuComPort)
    # exampleMessage = get_example_program_message()
    with serial.Serial(str(externalComPort), 115200, timeout=0) as serExternal:
        time.sleep(2)
        serExternal.flushInput()
        serExternal.flushOutput()
        # serExternal.set_buffer_size(rx_size=100000, tx_size=100000)
        print("External computer serial connected")
        time.sleep(0.5)
        with serial.Serial(str(mcuComPort), 115200, timeout=0) as serMCU:
            time.sleep(2)
            serMCU.flushInput()
            serMCU.flushOutput()
            # serMCU.set_buffer_size(rx_size=100000, tx_size=100000)
            print("MCU serial connected")
            time.sleep(0.5)

            while True:
                time.sleep(0.02)
                readNewMessagesFromACOMM(serExternal, serMCU, bb_ecc)
                time.sleep(0.02)
                readNewMessageFromExternalComputer(serExternal, serMCU, bb_ecc)
