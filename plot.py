#!/usr/bin/python

import sys
from decimal import *
import matplotlib.pyplot as plt
import csv

x = []
y = []


with open(str(sys.argv[1]),'r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        x.append(int(row[0]))
        y.append(Decimal(row[1]))

data=str(sys.argv[1])
data=data[data.find('/')+1:]
title = data + " vs Lifetime"
plt.scatter(x,y, label=title)
plt.xlabel('Lifetime')
plt.ylabel(data)
plt.legend()
plt.show()

