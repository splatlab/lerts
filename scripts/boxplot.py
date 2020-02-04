#!/usr/bin/python

import sys
import matplotlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 75})

infile1=str(sys.argv[1])
infile2=str(sys.argv[2])
infile3=str(sys.argv[3])
# infile4=str(sys.argv[4])

df1 = pd.read_csv(infile1, sep=',')
df2 = pd.read_csv(infile2, sep=',')
df3 = pd.read_csv(infile3, sep=',')
# df4 = pd.read_csv(infile4, sep=',')
# df = pd.concat([df1,df2,df3,df4])
df = pd.concat([df1,df2,df3])

# colors = ['pink', 'lightblue', 'lightgreen', 'red']
boxprops = dict(linewidth=5, color='darkgoldenrod')
medianprops = dict(linewidth=5, color='firebrick')
whiskerprops=dict(linewidth=5, color='green')
capprops=dict(linewidth=5, color='black')



bp = df.boxplot(by='structure', showfliers=False, boxprops=boxprops,
        medianprops=medianprops, whiskerprops=whiskerprops, capprops=capprops)

# for patch, color in zip(bp['boxes'], colors):
        # patch.set_facecolor(color) 

axes = plt.gca()
data=[2.2,0.9,0.8]
axes.bar(range(1, 4), data, color='yellow', align='center')
## Custom x-axis labels
# axes.set_xlabel('Distributions')
# axes.set_xlabel('Data structures')
axes.set_xlabel('Buffering approach')
# axes.set_xticklabels(['CF', 'CSL', 'CSL(cones)', 'CSL(cones-threads)'])
# axes.set_xticklabels(['CF', 'TSL', 'TSL(cones)', 'TSL(cones-threads)'])
# axes.set_xticklabels(['TSL1', 'TSL2', 'TSL3', 'TSL4'])
axes.set_xticklabels(['CSL Buffer', 'CSL Buffer-count', 'CSL No-buffer'])
# axes.set_xticklabels(['24M', '24M-23', 'Round-robin', 'Count-UR'])
# axes.margins(y=0)

# plt.title('stretch')
plt.title('Count stretch')
# plt.title('Time stretch')
# plt.suptitle('')
# plt.show()
fig = bp.get_figure()
fig.savefig('buffer_analysis')
