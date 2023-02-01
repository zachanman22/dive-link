from base64 import encode
import time
import serial
import json
import socket
from bluebuzz_ecc import BlueBuzz_ECC


def conditionMessageForComputerPublication(message):
    # decode message from MCU for computer publication
    # if message is to be transmitted, encode and return with True resposne
    # returns (encodedMessage, response)
    try:
        decodedJson = json.loads(message)
        decodedMessages = bb_ecc.decodeJson(decodedJson)
        print("Decoded messages: ", decodedMessages)
        if decodedMessages is None or len(decodedMessages) == 0:
            return (None, False)
        # decodedMessages.sort(key=len, reverse=True)
        messageToPublish = (decodedMessages[0])
        print("Messages to publish: ", messageToPublish)
        return (messageToPublish, True)
    except:
        print("Message failed")
        return (None, False)


def conditionMessageForMCUPublication(message):
    # encode the message for MCU publication
    # if message is a config update, test it before sending to MCU
    # if message is to be transmitted, encode and return with True resposne
    # returns (encodedMessage, response)
    try:
        if not is_message_config(message):
            encodedMessage = bb_ecc.encode(message)
            encodedMessageBstring = (encodedMessage + "\n").encode()
            # print("encodedMessageBstring:", encodedMessageBstring)
            return (encodedMessageBstring, True)
        else:
            decodedMessage = message.decode('utf8')
            if decodedMessage[-1] != '\n':
                message = (decodedMessage + '\n').encode()
            print("Config message type:", type(message))
            print("Config Message:", message)
            return (message, True)
    except:
        return (None, False)


def transmitMessageFromExternalComputer(ser, message, bb_ecc: BlueBuzz_ECC):
    encodedMessage = bb_ecc.encode(message)
    encodedMessageBstring = (encodedMessage + "\n").encode()
    print(encodedMessageBstring)
    ser.write(encodedMessageBstring)


def correctComPortForWindows(comport):
    # correct com name to properly allow window to connect
    if "COM" in comport:
        return "\\\\.\\" + comport
    return comport


def get_udp_server_address(sock):
    try:
        data, addr = sock.recvfrom(2048)  # buffer size is 1024 bytes
        if data.decode('utf-8') == 'server':
            print("server received from ", addr[0])
            return addr[0]
    except:
        return None


def connect_to_socket_port(UDP_IP, UDP_PORT):
    # connect to port, continue to try to connect until success
    while True:
        try:
            sock = socket.socket(socket.AF_INET,  # Internet
                                 socket.SOCK_DGRAM)  # UDP
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind((UDP_IP, UDP_PORT))
            sock.settimeout(0.005)
            print("Connected to ", UDP_IP, " on port ", UDP_PORT)
            return sock
        except:
            print("Port connection failed, retrying in 10 seconds...")
            time.sleep(10)


def connect_to_serial_port(serialPortString, description=None):
    while True:
        try:
            comPortCorrected = correctComPortForWindows(serialPortString)
            serGeneral = serial.Serial(comPortCorrected, 115200, timeout=0)
            # serGeneral.set_buffer_size(rx_size=100000, tx_size=100000)
            time.sleep(2)
            serGeneral.flushInput()
            serGeneral.flushOutput()
            print("Serial ", serialPortString,
                  " (", description, ") connected")
            time.sleep(0.5)
            return serGeneral
        except:
            if description is not None:
                print("Serial to ", description,
                      " failed, retrying in 10 seconds...")
            else:
                print("Serial to unknown port failed, retrying in 10 seconds...")
            time.sleep(10)


def is_message_config(message):
    global volume, baud, responseMessages, bb_ecc
    try:
        configJson = json.loads(message)
        if "bluebuzz" in configJson.keys():
            if "volume" in configJson.keys():
                volume = configJson["volume"]
            if "baud" in configJson.keys():
                baud = configJson["baud"]
            if "rscodec" in configJson.keys() and configJson["rscodec"] >= 0 and int(configJson["rscodec"]) % 2 == 0:
                bb_ecc.set_rscoded_size(int(configJson["rscodec"]))
            if "messageBreakLength" in configJson.keys() and configJson["messageBreakLength"] >= 8:
                bb_ecc.set_message_break_length(
                    int(configJson["messageBreakLength"]))
            if "response" in configJson.keys():
                responseMessages.append(json.dumps(
                    {"volume": volume, "baud": baud}, separators=(',', ':')).encode('utf8'))
            return True
    except:
        pass
    return False


volume = 0.01
baud = 200
responseMessages = []
if __name__ == "__main__":
    # set the encoding level for the BlueBuzz_ECC
    bb_ecc = BlueBuzz_ECC(hammingSize=0, rscodecSize=10, messageBreakLength=32)

    # set ip and port to listen/send on
    UDP_IP = ''
    # UDP_PORT = 8888
    receive_port = 8888
    transmit_port = 8887

    # server = get_udp_server_address(UDP_IP, UDP_PORT)
    serverIP = None
    sock = connect_to_socket_port(UDP_IP, receive_port)

    # set serial ports
    mcuComPort = "/dev/ttyACM0"
    externalComPort = "/dev/ttyUSB0"

    # connect to serial ports available
    serMCU = None
    serExternal = None

    while True:
        try:
            # connect to serMCU and serExternal, reconnect if port dropped
            # close serMCU port if open
            try:
                serMCU.close()
                print("Closing serial MCU port to reconnect")
                time.sleep(1)
            except:
                pass
            # close serExternal port if open
            try:
                serExternal.close()
                print("Closing serial external port to reconnect")
                time.sleep(1)
            except:
                pass

            # serMCU and serExternal connect to ports, returns None if port unavailable
            serMCU = connect_to_serial_port(mcuComPort, "MCU port")
            serExternal = connect_to_serial_port(
                externalComPort, "External port")
            # http://192.168.1.179/
            while True:
                time.sleep(0.005)
                # process messages from MCU ser -> external computer ser and sock. Try/Excepts are to catch USB failures and reboot
                messagesReceived = None
                try:
                    messagesReceived = serMCU.readline()
                    if len(messagesReceived) != 0:
                        conditionedMessage, response = conditionMessageForComputerPublication(
                            messagesReceived)
                        if response:
                            print("Received: ", conditionedMessage)
                            if serverIP is not None:
                                sock.sendto(conditionedMessage,
                                            (serverIP, transmit_port))
                            serExternal.write(conditionedMessage)
                except:
                    print(
                        "USB serial connection issue. Retrying to connect in 5 seconds...")
                    time.sleep(5)
                    raise RuntimeError(
                        "USB serial connection issue. Retrying to connect in 5 seconds...")

                # process messages from external computer ser -> mcu ser. Try/Excepts are to catch USB failures and reboot
                try:
                    messagesReceived = serExternal.readline()
                    if len(messagesReceived) != 0:
                        print("From ser ext: ", messagesReceived)
                        conditionedMessage, response = conditionMessageForMCUPublication(
                            messagesReceived)
                        if response:
                            serMCU.write(conditionedMessage)
                except:
                    pass

                # process messages from computer sock -> mcu ser
                if serverIP is not None:
                    try:
                        messagesReceived, addr = sock.recvfrom(
                            2048)  # buffer size is 1024 bytes
                        print("From computer sock:", messagesReceived)
                        if messagesReceived == b'server':
                            raise ValueError("ignore server ping")
                        conditionedMessage, response = conditionMessageForMCUPublication(
                            messagesReceived)
                        if response:
                            serMCU.write(conditionedMessage)
                    except:
                        pass
                else:
                    serverIP = get_udp_server_address(sock)

                # process any requested response messages bluebuzz -> external comp
                if len(responseMessages) > 0:
                    try:
                        responseMessage = responseMessages.pop()
                        print("response message requested: ", responseMessage)
                        print(type(responseMessage))
                        if serverIP is not None:
                            sock.sendto(responseMessage,
                                        (serverIP, transmit_port))
                        serExternal.write(responseMessage)
                    except:
                        pass
        except:
            print("error somewhere, attempting to restart in 5 seconds")
            time.sleep(3)
