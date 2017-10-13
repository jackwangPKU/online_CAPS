#!usr/bin/python
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import numpy as np
jet=plt.get_cmap('jet')
colors = iter(jet(np.linspace(0,1,10)))
ax = plt.subplot(1,1,1)
#plt.ion()
for i in range(int(sys.argv[2])):
	filename = './'+sys.argv[1]+str(i)+'.mrc'
	target = open(filename)
	data = []
	for line in target:
		a = float(line.strip().split()[0])
		data.append(a)
	#plt.figure(figsize(100,80))
	ax.plot(data,label='phase '+str(i),color=next(colors))
	#plt.savefig(sys.argv[1]+i+'.png')
	#plt.pause(0.05)
plt.title(sys.argv[1])
plt.axis([0,20000,0,1])
plt.xlabel("Cache size(KB)")
plt.ylabel("Miss rate");
box = ax.get_position()
ax.set_position([box.x0,box.y0,box.width*0.8,box.height])

plt.legend(loc='center left',bbox_to_anchor=(1,0.5))

plt.savefig(sys.argv[1]+'.png')
	#plt.show()
