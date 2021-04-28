# Predmet IMR
# Autor: Adam Fabo

import re
from collections import Counter

f = open("code2")

stuff = f.read()

x = re.findall("4,[0-9]*,[0-9]*",stuff)


tones = []
delays = []
for line in x:
    y = line.split(",")

    tones.append(int(y[1]))
    delays.append( round(int(y[2])/150))

delays = delays[:-1]
tones = tones[:-1]

print(delays)

delays_compress = []
counter = 0
x = 0
for d in delays:
    x += d*(16**counter)
    #print(x)
    counter +=1

    if counter ==4 :
        counter = 0

        delays_compress.append(x)
        x = 0

print((tones))
print(delays_compress)













