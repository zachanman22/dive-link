samp_freq = 200000
filePath = './transducer/tests/acoustic_tank/3-17-22/20/6m/baud_20__2000gain__34_40k__0.5_1647625775'
# transducer\tests\acoustic_tank\3-17-22\20\6m\baud_20__2000gain__34_40k__0.5_1647625775
# transducer\tests\acoustic_tank\3-17-22\20\6m\baud_20__2000gain__34_40k__0.5_1647623006
# transducer\tests\acoustic_tank\3-17-22\20\6m\baud_20__2000gain__34_40k__0.5_1647625252
loglist = None
startTime = 0
with open(filePath, 'r') as log:
    loglist = log.readlines()
    loglist = [x.split('\n')[0] for x in loglist]
    loglist = [x.split(' ') for x in loglist]
    loglist = [[int(j) for j in x] for x in loglist]
    startTime = loglist[0][0]
    # loglist = [[int(round((x[0]-startTime)/(1/samp_freq*1000000))), x[1]] for x in loglist]


x = 0
while x < len(loglist)-1:
    # print(loglist[x])
    if loglist[x][0] == loglist[x+1][0]:
        loglist.pop(x)
        continue
    # if loglist[x+1][0] < loglist[x][0]:
    #     loglist.pop(x+1)
    #     continue
    # if loglist[x][0] != loglist[x+1][0]-1:
    #     # print("Loglist ", loglist[x][0])
    #     # print("Loglist Higher ", loglist[x+1][0])
    #     loglist.insert(x+1,[loglist[x][0]+1, int((loglist[x][1]+loglist[x+1][1])/2)])
    #     print(loglist[x][0]+1)
    if loglist[x+1][0] - loglist[x][0] > 20:
        print(x)
    x = x + 1

# loglist = [[int(x[0]*(1/samp_freq*1000000)+startTime), x[1]]for x in loglist]
with open(filePath + '_fixed', 'w') as log:
    for x in loglist:
        tempstring = str(x[0]) + ' ' + str(x[1]) + '\n'
        log.write(tempstring)
print("done")