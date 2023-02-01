import random

def generateRandomList(size):
    return [0 if random.random() <= 0.5 else 1 for x in range(size)]

numberOfMessages = 1000
size = 2000
messageList = [generateRandomList(size) for x in range(numberOfMessages)]

folderPath = "python\\messagesToSend\\"
fileName = "messagesToSend"

with open(folderPath+fileName, "w") as f:
    for i in range(numberOfMessages):
        for x in range(size):
            f.write(str(messageList[i][x]))
        f.write('\n')

