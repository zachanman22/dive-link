import time

import numpy as np




def h(ret):
    for i in range(2**12):

        ret += b'a' * 2**12




def g(numpy_buffer):

    for i in range(2**12):
        numpy_buffer[i*(2**12):(i+1)*(2**12)] = np.frombuffer(b'a' * (2**12), dtype='b')
        
    print(b''.join(numpy_buffer.view('S1')[:100]))





numpy_buffer = np.zeros((2**24), dtype='b')

ret = bytearray()

timeStamp = time.time()

h(ret)

timeStamp2 = time.time()

g(numpy_buffer)

timeStamp3 = time.time()

print("H:",timeStamp2-timeStamp)

print("G:",timeStamp3-timeStamp2)