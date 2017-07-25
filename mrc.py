#!usr/bin/python
#import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import numpy as np
ax = plt.subplot(1,1,1)
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
#plt.figure(figsize(100,80))
plt.title(sys.argv[1]+' L3')
ax.plot(data1,label='Offline full',color='r')
ax.plot(data2,label='Offline 1/10',ls=':',color='y')
ax.plot(data3,label='Offline 1/100',ls=':',color='g')
ax.plot(data4,label='Offline 1/1000',ls=':',color='grey')
ax.plot(data5,label='Online 1/1',color='b')
plt.axis([0,20000,0,1])
plt.xlabel("Cache size(KB)")
plt.ylabel("Miss rate");
box = ax.get_position()
ax.set_position([box.x0,box.y0,box.width*0.8,box.height])

plt.legend(loc='center left',fontsize='small',bbox_to_anchor=(1,0.5))

plt.savefig(sys.argv[1]+'_l3_mrc.jpg')
#plt.show()
