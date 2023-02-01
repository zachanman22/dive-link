import os
import sys
import random
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"  # nopep8
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # nopep8
from bitstring import BitArray
from base64 import decode
from Hamming import Hamming
import pygame
import json
import serial
import time


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


def convertBitListToBitString(bitlist, addNewLine=False):
    bitstring = "".join([str(x) for x in bitlist])
    if (addNewLine):
        bitstring += '\n'
    bitstring = bitstring.encode()
    return bitstring


def sendMessage(messageDict, hamm, ser, verbose=False):
    bitlist = convertJSONtoBitList(messageDict)
    encodedMessageList = encodeMessage(bitlist, hamm)
    encodedMessageBitString = convertBitListToBitString(
        encodedMessageList, True)
    if ser != None:
        ser.write(encodedMessageBitString)
    if verbose:
        print(messageDict, encodedMessageBitString)


heading = 0
headingGain = 1


def updateHeading(axisValue):
    global heading, headingGain
    axisValue = axisValue*headingGain
    heading += axisValue
    while heading >= 256:
        heading -= 256
    while heading < 0:
        heading += 256

def get_program_message():
    global heading
    return {'h':int(heading), 'f':8, 'u':0}

# def get_heading_start_message():
#     return {'h':int(heading), 'f':0, 'u':8}

def mapping(axesData):
    # Left Joystick
    # Axis 0: Left/Right [-1,1]
    # Axis 1: Up/Down [-1,1]
    # Right Joystick
    # Axis 2: Left/Right [-1,1]
    # Axis 3: Up/Down [-1,1]
    # Triggers
    # Axis 4: Right Trigger, open to full pressed [-1,1]
    # Axis 5: Left Trigger, open to full pressed [-1,1]
    forward = 0
    up = 0
    updateHeading(axis[0])
    if (axis[4] > -0.99):
        forward = -(axis[4]+1)/2
    else:
        forward = (axis[5]+1)/2
    forward = int(round(forward*7))

    up = int(round(-1*axis[3]*7))

    return {"h": int(heading) % 256, "f": forward, "u": up}


if __name__ == "__main__":
    hammingSize = 8
    hamm = Hamming(hammingSize)

    comPort = "COM5"

    baudrate = 100
    volume = 0.1
    bitsPerMessage = 26
    bitsPerMessage += 1
    relayResetTimeMillis = 50
    timeBufferMillis = 100
    activeTransmit = True
    timeBetweenMessage = bitsPerMessage/baudrate + 2 * \
        relayResetTimeMillis/1000 + timeBufferMillis/1000
    print("Time Between Messages: ", timeBetweenMessage)

    pygame.init()
    # Loop until the user clicks the close button.
    done = False
    # Used to manage how fast the screen updates.
    clock = pygame.time.Clock()
    # Initialize the joysticks.
    pygame.joystick.init()
    lastMessageSent = time.time()
    previousButtonData = None
    programMode = False
    ser = None
    verbose = True
    while True:# with serial.Serial("\\\\.\\"+ str(comPort), 115200, timeout=0) as ser:
        # time.sleep(3)
        # ser.set_buffer_size(rx_size = 100000, tx_size = 100000)
        # print("Serial connected")
        # time.sleep(0.5)
        # -------- Main Program Loop -----------
        while not done:
            
            #
            # EVENT PROCESSING STEP
            #
            # Possible joystick actions: JOYAXISMOTION, JOYBALLMOTION, JOYBUTTONDOWN,
            # JOYBUTTONUP, JOYHATMOTION
            if pygame.QUIT in [event.type for event in pygame.event.get()]:
                done = True

            

            # Get count of joysticks.
            joystick_count = pygame.joystick.get_count()

            # For each joystick:
            for i in range(joystick_count):
                joystick = pygame.joystick.Joystick(i)
                joystick.init()

                buttons = joystick.get_numbuttons()
                buttonData = [joystick.get_button(i) for i in range(buttons)]
                a_button_pressed = buttonData[0]
                b_button_pressed = buttonData[1]
                x_button_pressed = buttonData[2]
                y_button_pressed = buttonData[3]

                if previousButtonData != None:
                    if b_button_pressed == 0 and previousButtonData[1] == 1:
                        activeTransmit = not activeTransmit
                    elif y_button_pressed == 0 and previousButtonData[3] == 1 and not programMode:
                        programMode = True
                    elif x_button_pressed == 0 and previousButtonData[2] == 1 and programMode:
                        programMode = False
                previousButtonData = buttonData

                # if programMode:
                #     send_program_message(hamm, ser)
                #     continue

                if not programMode and activeTransmit:
                    axes = joystick.get_numaxes()
                    axis = [joystick.get_axis(i) for i in range(axes)]
                    value = mapping(axis)
                    print(value)
                elif programMode:
                    value = get_program_message()
                    print("Program Mode Enabled: ", value)
                else:
                    print("Transmit Disabled")

                # if not programMode and activeTransmit:
                #     print(value)
                # elif programMode:
                #     print("Program Mode Enabled: ", value)
                # if not activeTransmit:
                #     print("Transmit Disabled")
                

                # strval = json.dumps(value)
                if time.time() - lastMessageSent >= timeBetweenMessage and (activeTransmit or programMode):
                    sendMessage(value, hamm, ser, verbose)
                    lastMessageSent = time.time()
                
            
            clock.tick(20)
            # Limit to 20 frames per second.
            

        # Close the window and quit.
        # If you forget this line, the program will 'hang'
        # on exit if running from IDLE.
        pygame.quit()

    # print("decoding test")
    # decodedBitTuple = decodeMessage(encodedMessageBitString, hamm)
    # if (len(decodedBitTuple[2]) == 0):
    #     decoded_dict = convertDecodedBitListToJSON(decodedBitTuple[0])
    #     print(decoded_dict)
