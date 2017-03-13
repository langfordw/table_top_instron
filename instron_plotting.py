import numpy as np
from matplotlib import pyplot as plt

datafile = "sarrus_data_1.csv"
print ("Importing data from " + datafile + "...")
data = np.loadtxt(datafile,delimiter=',')

datafile = "sarrus_data_2.csv"
print ("Importing data from " + datafile + "...")
data2 = np.loadtxt(datafile,delimiter=',')

plt.figure()
plt.plot((data[:,0]-6000)/2,data[:,1])
plt.plot(data2[:,0],data2[:,1])
plt.show()