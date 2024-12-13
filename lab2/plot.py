import matplotlib.pyplot as plt
import numpy as np
from itertools import chain
def setBeautifulGrid ():
    plt.grid(which='major',
            color = 'k',
            linewidth = 1)
    plt.minorticks_on()
    plt.grid(which='minor',
            color = 'k',
            linestyle = ':')
    
for B in chain (np.arange (0.1, 1.1, 0.1), [1.3, 1.4, 1.45, 1.49, 1.5, 1.51, 1.55, 1.6, 1.7]):
    x = list (map (float, input().split()))
    y = list (map (float, input().split()))

    plt.figure()
    setBeautifulGrid()
    plt.plot (x, y, "-o", markersize=2)
    plt.title (f"B = {B}")
    ax = plt.gca()
    if B < 1.1:
        ax.set_ylim([0, 1])
    else:
        ax.set_ylim([1, 1.7])
    plt.show()
