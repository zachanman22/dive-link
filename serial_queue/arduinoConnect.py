import serial
import time
import sys
import serial
import glob
import json

def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

#Find Serial Ports
sports = serial_ports()
print(sports)

ser  = serial.Serial(port = 'COM11', baudrate= 9600,
           timeout=1)
ser.close()
time.sleep(0.2)
ser.open()

while(True):
    #ubar json formatting  
    #time.sleep(0.03)
    while(ser.in_waiting):
        char = ser.readline()
        print(char)
        #time.sleep(0.1)

"""
if __name__ == '__main__':
    ser = serial.Serial('/dev/serial0', 9600, timeout=1)
    ser.reset_input_buffer()
    while True:
        ser.write(b"Hello from Raspberry Pi!\n")
        time.sleep(1)
"""