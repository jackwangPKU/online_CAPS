#!tusr/bin/python
#import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
filename1 = './'+sys.argv[1]+'_l3_mrc.txt'
filename2 = './'+sys.argv[1]+'_10_l3_mrc.txt'
filename3 = './'+sys.argv[1]+'_100_l3_mrc.txt'
filename4 = './'+sys.argv[1]+'_1000_l3_mrc.txt'
filename5 = './'+sys.argv[1]+'_on_mrc.txt'
target1 = open(filename1)
target2 = open(filename2)
target3 = open(filename3)
target4 = open(filename4)
target5 = open(filename5)

data1 = []
data2 = []
data3 = []
data4 = []
data5 = []

for line in target1:
    a = float(line.strip().split()[0])
    data1.append(a)


for line in target2:
    a = float(line.strip().split()[0])
    data2.append(a)

for line in target3:
    a = float(line.strip().split()[0])
    data3.append(a)

for line in target4:
    a = float(line.strip().split()[0])
    data4.append(a)

for line in target5:
    a = float(line.strip().split()[0])
    data5.append(a)

plt.title(sys.argv[1]+' L3')
plt.plot(data1,label='Offline full',ls=':',color='r')
plt.plot(data2,label='offline 1/10',ls=':',color='y')
plt.plot(data3,label='offline 1/100',ls=':',color='g')
plt.plot(data4,label='offline 1/1000',ls=':',color='grey')
plt.plot(data5,label='Online',color='b')
plt.axis([0,20000,0,1])
plt.xlabel("Cache size(KB)")
plt.ylabel("Miss rate");
plt.legend()
plt.savefig(sys.argv[1]+'_l3_mrc.jpg')
#plt.show()
