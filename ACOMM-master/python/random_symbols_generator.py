import random

if __name__ == '__main__':
    numberOfSymbols = 399
    
    listOfSymbols = []
    for _ in range(numberOfSymbols):
        listOfSymbols.append(str(random.randint(0,1)))
    print("".join(listOfSymbols))