from pickle import NONE
from BlueBuzz import BlueBuzz
import copy
import time

commConfig = {"comm": "serial",
              "baud": 115200,
              "port": "COM6"}
bluebuzz = BlueBuzz(commConfig)

modemConfig = copy.deepcopy(BlueBuzz.modem_config_default)
modemConfig["baud"] = 200
modemConfig["volume"] = 0.25
modemConfig["modulation"] = BlueBuzz.modulationOptions[1]
bluebuzz.update_modem(modemConfig)
time.sleep(1)
print(bluebuzz.receive_all().decode('utf-8'))
