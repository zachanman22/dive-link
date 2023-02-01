import json
import serial
import time
import copy


class BlueBuzz():

    commOptions = ("serial",)
    modulationOptions = ("fh-bfsk", "pask")
    baudOptions = (20, 50, 80, 100, 125, 160, 200, 250)
    serial_baudrate_options = (
        9600, 19200, 28800, 38400, 57600, 76800, 115200, 230400, 460800, 576000, 921600)
    recommendedFrequenciesAndRelativeVolumes = ([25000, 0.65],
                                                [33000, 0.45],
                                                [28000, 0.4],
                                                [36000, 0.45],
                                                [26000, 0.4],
                                                [34500, 0.5],
                                                [30000, 0.4],
                                                [36500, 0.6],
                                                [27000, 0.3],
                                                [35500, 0.55],
                                                [32500, 0.3],
                                                [42000, 0.8])

    modem_config_default = {"modulation": modulationOptions[0],
                            "baud": baudOptions[-1],
                            "volume": 1.0,
                            "freqs_and_r_volume": copy.deepcopy(list(recommendedFrequenciesAndRelativeVolumes))}

    # commConfig
    # {"comm":"serial", "port":"COM9", "baud":115200}
    def __init__(self, commConfig: dict, modemConfig: dict = None):
        # set modem config to default if not passed in
        if modemConfig is None:
            self.modemConfig = copy.deepcopy(BlueBuzz.modem_config_default)
        else:
            self.modemConfig = copy.deepcopy(modemConfig)

        # make sure the comm key exists
        if 'comm' not in commConfig.keys():
            raise ValueError("comm does not match these values: " +
                             ",".join([str(x) for x in BlueBuzz.commOptions]))

        # check if the comm selected is in the available options
        if commConfig['comm'] in BlueBuzz.commOptions:
            self.comm = commConfig['comm']
        else:
            raise ValueError("comm does not match these values: " +
                             ",".join([str(x) for x in BlueBuzz.commOptions]))

        # setup communication
        if self.comm == "serial":
            self.__set_serial_comm(commConfig)
        elif self.comm == "spi":
            self.__set_spi_comm(commConfig)

    def __set_modem_to_mcu_serial_baud(self, commConfig: dict):
        self.baud = 115200
        if 'baud' in commConfig.keys():
            if commConfig['baud'] in BlueBuzz.serial_baudrate_options:
                self.baud = commConfig['baud']
            else:
                raise ValueError("Baud rate not in serial options: " +
                                 ",".join([str(x) for x in BlueBuzz.serial_baudrate_options]))

    def __set_modem_to_mcu_serial_timeout(self, commConfig: dict):
        self.timeout = 0
        if 'timeout' in commConfig.keys():
            if 0 <= commConfig['timeout'] <= 1:
                self.timeout = commConfig['timeout']
            else:
                raise ValueError("Timeout value not between 0 and 1")

    def __set_serial_comm(self, commConfig: dict):
        if "port" not in commConfig.keys():
            raise ValueError("port is required for serial communication")
        if "COM" in commConfig['port']:
            commConfig['port'] = "\\\\.\\" + str(commConfig['port'])
        self.port = commConfig['port']
        self.__set_modem_to_mcu_serial_baud(commConfig)
        self.__set_modem_to_mcu_serial_timeout(commConfig)

        # try to connect to serial ports
        # attempt 3 times, and raise error if fail
        attempts = 3
        while attempts > 0:
            try:
                self.ser = serial.Serial(
                    self.port, self.baud, timeout=self.timeout)
                attempts = 0
            except:
                attempts -= 1
                if attempts <= 0:
                    raise AttributeError("Serial failed to connect")
            time.sleep(2)

    # update the modem config from a new dictionary
    # checks whether modem is valid before changing
    # returns False if not valid, True if valid
    def update_modem(self, modemConfig: dict):
        isConfigValid = self.__validate_new_modem_config(modemConfig)
        if not isConfigValid:
            return False

        # verify if baud rate updated, check if valid
        if "baud" in modemConfig.keys():
            self.modemConfig['baud'] = modemConfig['baud']

        # verify if modulation rate updated, check if valid
        if "modulation" in modemConfig.keys():
            self.modemConfig['modulation'] = modemConfig['modulation']

        # verify if volume rate updated, check if valid
        if "volume" in modemConfig.keys():
            self.modemConfig['volume'] = modemConfig['volume']

        # verify if freqs_and_r_volume rate updated, check if valid
        if "freqs_and_r_volume" in modemConfig.keys():
            self.modemConfig['freqs_and_r_volume'] = modemConfig['freqs_and_r_volume']

        self.modemConfig['freqs_and_r_volume_size'] = len(
            self.modemConfig['freqs_and_r_volume'])

        # convert modem dictionary to json string
        modem_update_string = json.dumps(self.modemConfig)
        messageSuccess = self.__send_message_to_modem(modem_update_string)
        return messageSuccess

    # recieve new message
    # requires polling
    # only publishes the most recent message
    def receive_new_message(self):
        if self.comm == "serial":
            return self.ser.read_until('\n')
        return None

    # receive all publishes everything availble
    # requires polling
    def receive_all(self):
        if self.comm == "serial":
            return self.ser.read_all()
        return None

    # def transmit(bitmessage) which sends a string of "10101"s to the microcontroller
    # this is the public version that confirms the message is only 10101s
    def transmit(self, bit_message: str):
        for x in bit_message:
            if x != '0' and x != '1':
                return False
        self.__send_message_to_modem(bit_message)
        return True

    # transmit the message str directly to the MCU of the modem
    def __send_message_to_modem(self, message: str):
        if self.comm == "serial":
            self.ser.write((message+'\n').encode('utf-8'))
        return True

    def __validate_new_modem_config(self, modemConfig: dict):
        # verify if baud rate updated, check if valid
        if "baud" in modemConfig.keys():
            if modemConfig["baud"] not in BlueBuzz.baudOptions:
                return False

        # verify if modulation rate updated, check if valid
        if "modulation" in modemConfig.keys():
            if modemConfig["modulation"] not in BlueBuzz.modulationOptions:
                return False

        # verify if volume rate updated, check if valid
        if "volume" in modemConfig.keys():
            if not (0 <= modemConfig["volume"] <= 1):
                return False

        # verify if freqs_and_r_volume rate updated, check if valid
        try:
            if "freqs_and_r_volume" in modemConfig.keys():
                for x in modemConfig["freqs_and_r_volume"]:
                    if not (1 <= x[0] <= 100000):
                        return False
                    if x[1] < 0:
                        return False
        except:
            return False

        # return the the modem config is valid
        return True

    def set_spi_comm(self, commConfig):
        pass

    def __del__(self):
        try:
            if self.comm == "serial":
                self.ser.close()
        except:
            pass
