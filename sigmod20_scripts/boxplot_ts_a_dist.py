#!/usr/bin/python

import sys
import matplotlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 35})

infile1=str(sys.argv[1])
infile2=str(sys.argv[2])
infile3=str(sys.argv[3])
infile4=str(sys.argv[4])

df1 = pd.read_csv(infile1, sep=',')
df2 = pd.read_csv(infile2, sep=',')
df3 = pd.read_csv(infile3, sep=',')
df4 = pd.read_csv(infile4, sep=',')
df = pd.concat([df1,df2,df3,df4])

boxprops = dict(linewidth=5, color='darkgoldenrod')
medianprops = dict(linewidth=5, color='firebrick')
whiskerprops=dict(linewidth=5, color='green')
capprops=dict(linewidth=5, color='black')

bp = df.boxplot(by='structure', showfliers=True, boxprops=boxprops,
        medianprops=medianprops, whiskerprops=whiskerprops, capprops=capprops)

axes = plt.gca()
## Custom x-axis labels
axes.set_xlabel('Data structures')
axes.set_xticklabels(['TSL1', 'TSL2', 'TSL3', 'TSL4'])

plt.title('Time stretch')
plt.suptitle('')
fig=plt.gcf()
fig.set_size_inches(32,24)
# plt.show()
plt.savefig(sys.argv[5])
# fig.savefig(sys.argv[5])

