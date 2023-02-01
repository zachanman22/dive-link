import gzip
from compress import Compressor
import zlib
import json
import lzma

#
# binary_data = json.dumps(data)
# binary_data = binary_data.replace(" ", "")
# print(binary_data)
# binary_data_encoded = binary_data.encode('UTF-8')
# print(binary_data_encoded)
# zlibc = zlib.compress(binary_data.encode())
# lzmac = lzma.compress(binary_data.encode())
# print(zlibc)
# print(len(binary_data_encoded))
# print(len(zlibc))
# print("LZMA:", len(lzmac))
from reedsolo import RSCodec, ReedSolomonError
from bitstring import BitArray
import random
from Hamming import Hamming
import time
import ctypes
import binascii
import json

rscodecSize = 2
rsc = RSCodec(rscodecSize)  # 10 ecc symbols


data = bytearray([100, 20])
rscdata = rsc.encode(data)
rscdata[1] = 21
print(len(data))
datacompressed = zlib.compress(data)
print(datacompressed)
print(len(datacompressed))
print("rscdata:", rscdata)
print("len(rscdata):", len(rscdata))
print(rsc.decode(rscdata))

data = {"h": 200, "f": 7, "u": 6}
# for i in range(N):
#     uid = "whatever%i" % i
#     dv = [1, 2, 3]
#     data.append({
#         'what': uid,
#         'where': dv
#     })                                           # 1. data

json_str = json.dumps(data) + "\n"               # 2. string (i.e. JSON)
json_bytes = json_str.encode('utf-8')            # 3. bytes (i.e. UTF-8)
compressedbytes = gzip.compress(json_bytes)
# 4. fewer bytes (i.e. gzip)
print(len(json_bytes))
print(len(compressedbytes))
