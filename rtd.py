#!usr/bin/python
import matplotlib
import math
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import numpy as np

off = plt.subplot(2,1,1)
on = plt.subplot(2,1,2)

file1 = './'+sys.argv[1]+'_off_rtd.txt'
file2 = './'+sys.argv[1]+'_on_rtd.txt'

t1 = open(file1)
t2 = open(file2)

d1 = []
d2 = []
c1 = 0
c2 = 0
for line in t1:
    a = int(line.strip().split()[0])
    if a!=0:
        b = math.log(a,2)
        d1.append(b)
    else:
        d1.append(a)

for line in t2:
    a = int(line.strip().split()[0])
    if a!=0:
        d2.append(math.log(a,2))
    else:
        d2.append(a)

for dot in d1:
    off.plot([c1,c1],[0,dot],color='grey')
    c1 = c1+1
    
for dot in d2:
    on.plot([c2,c2],[0,dot],color='grey')
    c2 = c2+1

plt.title(sys.argv[1]+' RTD')
off.set_xlabel('reuse time distance')
off.set_ylabel('log(number of reference)')
off.set_title('Offline')
off.axis([0,10000,0,20])

on.set_xlabel('reuse time distance')
on.set_ylabel('log(number of reference)')
on.set_title('Online')
on.axis([0,10000,0,20])
plt.tight_layout()
plt.savefig(sys.argv[1]+'_rtd.jpg')
