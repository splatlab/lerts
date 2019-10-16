#!/usr/bin/python

import sys
import matplotlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 26})

infile1=str(sys.argv[1])
infile2=str(sys.argv[2])
infile3=str(sys.argv[3])
infile4=str(sys.argv[4])

df1 = pd.read_csv(infile1, sep=',')
df2 = pd.read_csv(infile2, sep=',')
df3 = pd.read_csv(infile3, sep=',')
df4 = pd.read_csv(infile4, sep=',')
df = pd.concat([df1,df2,df3,df4])
# df = pd.concat([df1,df2])

colors = ['pink', 'lightblue', 'lightgreen', 'red']
boxprops = dict(linestyle='--', linewidth=5, color='darkgoldenrod')
medianprops = dict(linestyle='-.', linewidth=5, color='firebrick')
whiskerprops=dict(linewidth=3, color='green')
capprops=dict(linewidth=5, color='black')

bp = df.boxplot(by='structure', showfliers=False,
        showmeans=True,meanline=True, boxprops=boxprops,
        medianprops=medianprops, whiskerprops=whiskerprops, capprops=capprops)

# for patch, color in zip(bp['boxes'], colors):
        # patch.set_facecolor(color) 

axes = plt.gca()
## Custom x-axis labels
axes.set_xlabel('Data structures')
# axes.set_xticklabels(['CF', 'PF', 'PF(cones)', 'PF(cones-threads)'])
# axes.set_xticklabels(['CF', 'TF', 'TF(cones)', 'TF(cones-threads)'])
axes.set_xticklabels(['TF1', 'TF2', 'TF3', 'TF4'])
# axes.margins(y=0)

# plt.title('Count stretch')
plt.title('Time stretch')
plt.suptitle('')
plt.show()
