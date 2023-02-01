from ast import excepthandler
import json


class Bluebuzz_Config():
    hamming_enabled: bool           # toggles hamming enabled
    hamming_ne_length: int          # not encoded hamming length
    rs_codec_size: int              # rs codec size for outer layer encoder
    expected_message_type: str     # expected message type is the response type when a raw message arrives (ascii, hex, bitstring_as_ascii, bitstring)
    error_response: bool            # ping back error message if incoming message fails
    baud: int                       # set baud rate (must match the preset baud values)
    frequencies_and_volumes: list   # example: [[25000,0.3],[26000,0.4],...]
    modulation_type: str             # fhfsk
    volume: float

    frequencies_and_volumes_default =  ([25000, 0.65],
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
    expected_message_type_values = ('utf-8', 'bitstring_as_utf-8')
    baudrate_values = (20, 50, 100, 125, 160, 200, 250)
    modulation_type_values = ('FHFSK', 'MASK')

    # def __init__(self):
    #     toDict = {"hamming_enabled": self.hamming_enabled,
    #             "hamming_ne_length": self.hamming_ne_length,
    #             "rs_codec_size": self.rs_codec_size,
    #             "expected_message_type": self.expected_message_type,
    #             "error_response": self.error_response,
    #             "baud": self.baud,
    #             "frequencies_and_volumes": self.frequencies_and_volumes,
    #             "modulationType": self.modulation_type,
    #             "volume": self.volume}

    def __init__(self):
        self.hamming_enabled = True
        self.hamming_ne_length = 8
        self.rs_codec_size = 2
        self.expected_message_type = Bluebuzz_Config.expected_message_type_values[0]
        self.error_response = False
        self.baud = Bluebuzz_Config.baudrate_values[0]
        self.frequencies_and_volumes = list(Bluebuzz_Config.frequencies_and_volumes_default)
        self.modulation_type = Bluebuzz_Config.modulation_type_values[0]
        self.volume = 1.0
        
        

    # hamming_enabled
    def _get_hamming_enabled(self):
        return self.__hamming_enabled
    def _set_hamming_enabled(self, value):
        if not isinstance(value, bool):
            raise TypeError("hamming_enabled must be type bool")
        self.__hamming_enabled = value
    hamming_enabled = property(_get_hamming_enabled, _set_hamming_enabled)

    # hamming_ne_length
    def _get_hamming_ne_length(self):
        return self.__hamming_ne_length
    def _set_hamming_ne_length(self, value):
        if not isinstance(value, int):
            raise TypeError("hamming_enabled must be type int")
        if  not (4 <= value <= 128):
            raise ValueError("Hamming not encoded length must be 4 <= x <= 128")
        self.__hamming_ne_length = value
    hamming_ne_length = property(_get_hamming_ne_length, _set_hamming_ne_length)

    # rs_codec_size
    def _get_rs_codec_size(self):
        return self.__rs_codec_size
    def _set_rs_codec_size(self, value):
        if not isinstance(value, int):
            raise TypeError("rs_codec_size must be type int")
        if  not (2 <= value <= 48):
            raise ValueError("rs_codec_size must be 2 <= x <= 48")
        self.__rs_codec_size = value
    rs_codec_size = property(_get_rs_codec_size, _set_rs_codec_size)

    # expected_message_type
    def _get_expected_message_type(self):
        return self.__expected_message_type
    def _set_expected_message_type(self, value):
        if not isinstance(value, str):
            raise TypeError("expected_message_type must be type str")
        if value not in Bluebuzz_Config.expected_message_type_values:
            raise ValueError("expected_message_type must be in: [" + ",".join([str(x) for x in Bluebuzz_Config.expected_message_type_values]) + "]")
        self.__expected_message_type = value
    expected_message_type = property(_get_expected_message_type, _set_expected_message_type)

    # error_response
    def _get_error_response(self):
        return self.__error_response
    def _set_error_response(self, value):
        if not isinstance(value, bool):
            raise TypeError("error_response must be type bool")
        self.__error_response = value
    error_response = property(_get_error_response, _set_error_response)

    # baud
    def _get_baud(self):
        return self.__baud
    def _set_baud(self, value):
        if not isinstance(value, int):
            raise TypeError("baud must be type int")
        if value not in Bluebuzz_Config.baudrate_values:
            raise ValueError("baud must be [" + ",".join([str(x) for x in Bluebuzz_Config.baudrate_values]) + "]")
        self.__baud = value
    baud = property(_get_baud, _set_baud)

    # frequencies_and_volumes
    def _get_frequencies_and_volumes(self):
        return self.__frequencies_and_volumes
    def _set_frequencies_and_volumes(self, value):
        if not isinstance(value, list):
            raise TypeError("baud must be type list")
        try:
            for x in value:
                if not (18000 <= x[0] <= 60000) or not(0 <= x[1] <= 1):
                    raise ValueError("issue with frequency/volume levels")
        except:
            raise ValueError("issue with frequency/volume levels")
        self.__frequencies_and_volumes = value
    frequencies_and_volumes = property(_get_frequencies_and_volumes, _set_frequencies_and_volumes)

    

    def __repr__(self):
        toDict = {"hamming_enabled": self.hamming_enabled,
                "hamming_ne_length": self.hamming_ne_length,
                "rs_codec_size": self.rs_codec_size,
                "expected_message_type": self.expected_message_type,
                "error_response": self.error_response,
                "baud": self.baud,
                "frequencies_and_volumes": self.frequencies_and_volumes,
                "modulationType": self.modulation_type,
                "volume": self.volume}
        return json.dumps(toDict)
    
