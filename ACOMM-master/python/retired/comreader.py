import time

with open('./transducer/tests/250/test1/baud_250__25_33k___1', 'w') as log:
    time.sleep(1)
    # Don't fight the Kernal, wait some seconds for prepare device

    with open("\\\\.\\COM4", "w") as readBuff:
        while flag:
            ans = readBuff.readline()
            if ans:
                if ans[:-1] == "OVER":
                        flag = 0
                else:
                    log.write(ans[:])
        readBuff.close()
    log.close()