import matplotlib.pyplot as plt
import pandas as pd
import math
import numpy as np
from scipy.stats import chi2

class KalmanFilter(object):
    def __init__(self, F = None, B = None, H = None, Q = None, R = None, P = None, x0 = None):
        """
        F - transition matrix
        B - control input model
        H - observation model
        Q - process noise covariance
        R - measurement noise covariance
        P - updated estimate covariance
        """

        if(F is None or H is None):
            raise ValueError("Set proper system dynamics.")

        self.n = F.shape[1]
        self.m = H.shape[1]

        self.F = F
        self.H = H
        self.B = 0 if B is None else B
        self.Q = np.eye(self.n) if Q is None else Q
        self.R = np.eye(self.n) if R is None else R
        self.P = np.eye(self.n) if P is None else P
        self.x = np.zeros((self.n, 1)) if x0 is None else x0

    def predict(self, u = 0):
        self.x = np.dot(self.F, self.x) + np.dot(self.B, u)
        self.P = np.dot(np.dot(self.F, self.P), self.F.T) + self.Q
        return self.x

    def update(self, z):
        y = z - np.dot(self.H, self.x)
        S = self.R + np.dot(self.H, np.dot(self.P, self.H.T))
        K = np.dot(np.dot(self.P, self.H.T), np.linalg.inv(S))
        self.x = self.x + np.dot(K, y)
        I = np.eye(self.n)
        self.P = np.dot(np.dot(I - np.dot(K, self.H), self.P),
                (I - np.dot(K, self.H)).T) + np.dot(np.dot(K, self.R), K.T)

    def distance(self, z, alpha):
        """
        Mahalanobis distance
        """
        y = z - np.dot(self.H, self.x)
        return np.linalg.norm(y)
        # S = self.R + np.dot(self.H, np.dot(self.P, self.H.T))
        # distance = math.sqrt((y.T @ np.linalg.inv(S) @ y)[0,0])

        # # https://stats.stackexchange.com/questions/97408/relation-of-mahalanobis-distance-to-log-likelihood
        # pvalue = 1 - chi2.cdf(alpha*distance, df=3)

        # return distance, pvalue

path = 'train_anomaly_5.csv'

df1, df2 = pd.read_csv(path+'1'), pd.read_csv(path+'2')

print(df1.head())
print(df2.head())

t1, y1 = df1['times'].to_numpy(), df1['P'].to_numpy()
t2, y2 = df2['times'].to_numpy(), df2['I'].to_numpy()

fig, axes = plt.subplots(nrows=2, ncols=2)
axes[0, 0].plot(t1, y1)
axes[1, 0].plot(t2, y2)

kalman1 = KalmanFilter(F=np.eye(2), H=np.eye(2), Q=np.array([
    [1.0, 0],
    [0, 0.001]
]))

kalman2 = KalmanFilter(F=np.eye(2), H=np.eye(2), Q=np.array([
    [1.0, 0],
    [0, 0.05]
]), x0=np.array([[0.0], [0.17]]))

kt, ky, d = [], [], []
for t, y in zip(t1, y1):
    z = np.array([[t], [y]], np.float32)
    dist = kalman1.distance(z, alpha=7.5)
    kalman1.update(z)
    pred = kalman1.predict()
    d.append(dist)
    kt.append(pred[0, 0])
    ky.append(pred[1, 0])
axes[0, 0].plot(kt, ky, color='green')
axes[0, 1].plot(kt, d)

kt, ky, d = [], [], []
for t, y in zip(t2, y2):
    z = np.array([[t], [y]], np.float32)
    dist = kalman2.distance(z, alpha=0.45)
    kalman2.update(z)
    pred = kalman2.predict()
    d.append(dist)
    kt.append(pred[0, 0])
    ky.append(pred[1, 0])
axes[1, 0].plot(kt, ky, color='green')
axes[1, 1].plot(kt, d)

plt.show()
