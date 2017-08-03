#!/usr/bin/python
#import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
filename1 = './'+sys.argv[1]+'.txt'
filename2 = './'+sys.argv[1]+'_2.txt'
target1 = open(filename1)
target2 = open(filename2)
data1 = []
data2 = []

for line in target1:
    a = float(line.strip().split()[0])
    data1.append(a)


for line in target2:
    a = float(line.strip().split()[0])
    data2.append(a)
plt.title(sys.argv[1])
plt.plot(data1,label='offline',color='r')
plt.plot(data2,label='online',color='b')
#plt.axis([0,600,0,0.8])
plt.xlabel("Cache size(KB)")
plt.ylabel("Miss rate");
plt.legend()
plt.savefig('init_'+sys.argv[1]+'.jpg')
#plt.show()
