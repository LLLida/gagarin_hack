import matplotlib.pyplot as plt
import pandas as pd
import math
import numpy as np
from scipy.stats import chi2

path = 'train_anomaly_5.csv'

df1, df2 = pd.read_csv(path+'1'), pd.read_csv(path+'2')

print(df1.head())
print(df2.head())

t1, y1 = df1['times'].to_numpy(), df1['P'].to_numpy()
t2, y2 = df2['times'].to_numpy(), df2['I'].to_numpy()

def ema(data, window):
    weights = np.exp(np.linspace(-1., 0., window))
    weights /= weights.sum()
    ema = np.convolve(data, weights, mode='full')[:len(data)]
    ema[:window] = ema[window]
    return ema

plt.show()
