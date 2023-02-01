from ast import excepthandler
import serial
import time
import pynmea2
import datetime
from pathlib import Path
from datetime import datetime
import json

def getLatLon(ser):
    try:
        data = ser.readall().decode()
    except:
        return []
    latlon_list = []
    for x in reversed(data.split('\r\n')):
        try:
            msg = pynmea2.parse(x)
            latitude = float(msg.lat)
            longitude = float(msg.lon)
            latlon_list.appen((latitude,longitude))
        except:
            pass
    return latlon_list



if __name__ == '__main__':
    gpsPort = 'COM8'
    acommPort = 'COM4'
    baud = 200
    message = '110110000101110001010001000010000010000110011000101000011101110000010110000000101110000010011101011101011100100010001110011111110000110011011010100011000011010000010001111011011011110111000111111011010000000010100111111000011010001010001100100110110011000011110001011110110110001101110010110000101101110100001111001110000111001011100010100000001001000100000100110110010001011010001100001001000001111'
    location = 'kraken_springs'
    time_between_msg = 15

    encodedMessage = (message+'\n').encode()

    gps_ser = serial.Serial("\\\\.\\"+ str(gpsPort), 9600, timeout=0.1)
    print("GPS Port Opened")
    time.sleep(2)
    acomm_ser = serial.Serial("\\\\.\\"+ str(acommPort), 115200, timeout=0)
    print("ACOMM Connected")
    time.sleep(2)

    date = datetime.now().date()
    folderPath = './transducer/tests/' + str(location) + '/'+str(date)+'/fhfsk/'
    Path(folderPath).mkdir(parents=True,exist_ok=True)

    transmit_file = folderPath + 'transmit_data.txt'
    f = open(transmit_file, "a")

    while True:
        latlon_list = getLatLon(gps_ser)
        if latlon_list is not None or len(latlon_list) != 0:
            acomm_ser.write(encodedMessage)
            dataToWrite = {"latlon": latlon_list,
                    "time": time.time(),
                    "baud": baud,
                    "message": message}
            f.write(json.dumps(dataToWrite) + '\n')
            time.sleep(time_between_msg + len(message)/baud)
        else:
            time.sleep(1)
        
        