#!/usr/bin/python3

import sys
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 35})

csl = float(sys.argv[1])
irl = float(sys.argv[2])
tsl1 = float(sys.argv[3])
tsl2 = float(sys.argv[4])
tsl3 = float(sys.argv[5])
tsl4 = float(sys.argv[6])

fig = plt.figure()
ax = plt.gca() 
ds = ['CSL', 'IRL', 'TSL1', 'TSL2', 'TSL3', 'TSL4']
tp = [csl,irl,tsl1,tsl2,tsl3,tsl4]
ax.bar(ds,tp)
ax.set_xlabel('Total Read')
ax.set_ylabel('I/O in GBs')
ax.set_xticklabels(['CSL', 'IRL', 'TSL1', 'TSL2', 'TSL3', 'TSL4'])
fig=plt.gcf()
fig.set_size_inches(32,24)
# plt.show()
plt.savefig(sys.argv[7])
# plt.savefig(sys.argv[8])

