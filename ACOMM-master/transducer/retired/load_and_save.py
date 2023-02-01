folderPath = ".\\transducer\\tests\\250\\"
fileName = "baud_250__25_33k___1"
newFileName = fileName+"__forArduino"
f = open(folderPath+fileName, "r")

writeFile = open(folderPath+newFileName, "w")
writeFile.write("{")
for i,x in enumerate(f):
    if(i > 170000 and i < 300000):
        writeFile.write((((x.split(" "))[1].split("\n"))[0])+",")
writeFile.write("};")
writeFile.close()