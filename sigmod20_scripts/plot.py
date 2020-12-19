#!/usr/bin/python

import sys
from decimal import *
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 35})
import csv

x = []
y = []


with open(str(sys.argv[1]),'r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        x.append(int(row[0]))
        y.append(Decimal(row[1]))

stretch="Count stretch"
# stretch="Time stretch"
title = stretch + " vs Lifetime"
plt.scatter(x,y, label=title)
plt.xlabel('Lifetime')
plt.ylabel(stretch + " (multiple threads)")
plt.legend()
plt.show()

