import time
import serial
import sys

if __name__ == '__main__':

    filepath = r"transducer\tests\kraken_springs\2022-09-05\fhfsk\100\1m\OP820\fhfsk__30000gain__10__sampleK_200_398190.txt"
    optionalFileName = "data_100_1"
    optionalFilePath = r"transducer\tests\data_to_transfer" + "\\"
    if optionalFileName is None:
        filepathFileIndex = filepath.rfind(".")
        filepath2 = filepath[:filepathFileIndex] + "_edit.csv"
    else:
        filepath2 = optionalFilePath + optionalFileName + ".csv"
    with open(filepath, 'r') as f:
        data = f.readlines()
    data = [x.split('\n')[0] for x in data]
    data = [x.split(' ') for x in data]
    data = [[int(x[0]),int(x[1])] for x in data]
    with open(filepath2, 'w') as f:
        for sample in data:
            f.write((str(sample[1])+'\n'))
    print("Complete")
    