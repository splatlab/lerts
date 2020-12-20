#!/usr/bin/python

import sys
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 35})

mg = float(sys.argv[1])
csl = float(sys.argv[2])
irl = float(sys.argv[3])
tsl1 = float(sys.argv[4])
tsl2 = float(sys.argv[5])
tsl3 = float(sys.argv[6])
tsl4 = float(sys.argv[7])

fig = plt.figure()
ax = plt.gca() 
ds = ['MG', 'CSL', 'IRL', 'TSL1', 'TSL2', 'TSL3', 'TSL4']
tp = [mg,csl,irl,tsl1,tsl2,tsl3,tsl4]
ax.bar(ds,tp)
ax.set_xlabel('Data Structures')
ax.set_ylabel('Insertion Throughput (Million ops/sec)')
ax.set_xticklabels(['MG', 'CSL', 'IRL', 'TSL1', 'TSL2', 'TSL3', 'TSL4'])
fig=plt.gcf()
fig.set_size_inches(32,24)
# plt.show()
plt.savefig(sys.argv[8])
# plt.savefig(sys.argv[8])

