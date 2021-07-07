#!/usr/bin/python3

import sys
import matplotlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 35})

infile1=str(sys.argv[1])
infile2=str(sys.argv[2])
infile3=str(sys.argv[3])

df1 = pd.read_csv(infile1, sep=',')
df2 = pd.read_csv(infile2, sep=',')
df3 = pd.read_csv(infile3, sep=',')
df = pd.concat([df1,df2,df3])

boxprops = dict(linewidth=5, color='darkgoldenrod')
medianprops = dict(linewidth=5, color='firebrick')
whiskerprops=dict(linewidth=5, color='green')
capprops=dict(linewidth=5, color='black')

bp = df.boxplot(by='structure', showfliers=True, boxprops=boxprops,
        medianprops=medianprops, whiskerprops=whiskerprops, capprops=capprops)

axes = plt.gca()
buffer_tp = float(sys.argv[4])
buffer_count_tp = float(sys.argv[5])
buffer_no_tp = float(sys.argv[6])
data=[buffer_tp,buffer_count_tp,buffer_no_tp]
axes.bar(range(1, 4), data, color='yellow', align='center')
## Custom x-axis labels
axes.set_xlabel('Buffering approach')
axes.set_xticklabels(['CSL Buffer', 'CSL Buffer-count', 'CSL No-buffer'])

plt.title('Count stretch')
plt.suptitle('')
fig=plt.gcf()
fig.set_size_inches(32,24)
# plt.show()
plt.savefig(sys.argv[7])
# fig.savefig(sys.argv[5])

