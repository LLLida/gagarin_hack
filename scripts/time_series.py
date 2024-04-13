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

    def distance(self, z):
        """
        Mahalanobis distance
        """
        y = z - np.dot(self.H, self.x)
        S = self.R + np.dot(self.H, np.dot(self.P, self.H.T))
        distance = math.sqrt((y.T @ np.linalg.inv(S) @ y)[0,0])

        # https://stats.stackexchange.com/questions/97408/relation-of-mahalanobis-distance-to-log-likelihood
        pvalue = 1 - chi2.cdf(100*distance, df=3)

        return distance, pvalue

class NewKalmanFilter:
    def __init__(self, n, m, A, B, H1, H2, Q, R, x_hat, P):
        self.n = n  # state dimension
        self.m = m  # measurement dimension
        self.A = A  # state transition matrix
        self.B = B  # input control matrix
        self.H1 = H1  # measurement matrix
        self.H2 = H2  # measurement matrix
        self.Q = Q  # process noise covariance matrix
        self.R = R  # measurement noise covariance matrix
        self.x_hat = x_hat  # state estimate
        self.P = P  # estimate covariance matrix

    def predict(self, u=0):
        self.x_hat = np.dot(self.A, self.x_hat) + np.dot(self.B, u)

        self.P = np.dot(np.dot(self.A, self.P), self.A.T) + self.Q

        return self.x_hat

    def update1(self, z):
        y = z - np.dot(self.H1, self.x_hat)
        S = np.dot(np.dot(self.H1, self.P), self.H1.T) + self.R
        K = np.dot(np.dot(self.P, self.H1.T), np.linalg.inv(S))
        self.x_hat = self.x_hat + np.dot(K, y)
        self.P = self.P - np.dot(np.dot(K, self.H1), self.P)

    def update2(self, z):
        y = z - np.dot(self.H2, self.x_hat)
        S = np.dot(np.dot(self.H2, self.P), self.H2.T) + self.R
        K = np.dot(np.dot(self.P, self.H2.T), np.linalg.inv(S))
        self.x_hat = self.x_hat + np.dot(K, y)
        self.P = self.P - np.dot(np.dot(K, self.H2), self.P)

    def distance1(self, z):
        y = z - np.dot(self.H1, self.x)
        S = self.R + np.dot(self.H1, np.dot(self.P, self.H1.T))
        distance = math.sqrt((y.T @ np.linalg.inv(S) @ y)[0,0])

        # https://stats.stackexchange.com/questions/97408/relation-of-mahalanobis-distance-to-log-likelihood
        # pvalue = 1 - chi2.cdf(100*distance, df=3)

        # return distance, pvalue
        return distance

    def distance2(self, z):
        y = z - np.dot(self.H2, self.x)
        S = self.R + np.dot(self.H2, np.dot(self.P, self.H2.T))
        distance = math.sqrt((y.T @ np.linalg.inv(S) @ y)[0,0])

        # https://stats.stackexchange.com/questions/97408/relation-of-mahalanobis-distance-to-log-likelihood
        # pvalue = 1 - chi2.cdf(100*distance, df=3)

        # return distance, pvalue
        return distance


path = 'train_anomaly_5.csv'

df1, df2 = pd.read_csv(path+'1'), pd.read_csv(path+'2')

print(df1.head())
print(df2.head())

t1, y1 = df1['times'].to_numpy(), df1['P'].to_numpy()
t2, y2 = df2['times'].to_numpy(), df2['I'].to_numpy()

fig, (ax1, ax2) = plt.subplots(nrows=2)
ax1.plot(t1, y1)
ax2.plot(t2, y2)

H1=np.array([
    [1, 0, 0],
    [0, 1, 0],
])
H2=np.array([
    [1, 0, 0],
    [0, 0, 1]
])
kalman = NewKalmanFilter(3, 2, A=np.eye(3), B=0, H1=H1, H2=H2, Q=np.array([
    [1.0, 0,     0],
    [0,   0.001, 0],
    [0,   0,     0.05]
]), R=np.eye(2), P=np.eye(3), x_hat=np.array([[0.0], [0.01], [0.17]]))

kt, kx, ky = [], [], []
dx, dy = [], []
j = 0
for i in range(len(t1)):
    if j<len(t2) and t1[i] >= t2[j]:
        z = np.array([[t2[j]], [y2[j]]])
        kalman.update2(z)
        j += 1
    z = np.array([[t1[i]], [y1[i]]], np.float32)

    kalman.update1(z)

    pred = kalman.predict()
    kt.append(pred[0, 0])
    kx.append(pred[1, 0])
    ky.append(pred[2, 0])
ax1.plot(kt, kx, color='green')
ax2.plot(kt, ky, color='green')

# kalman1 = KalmanFilter(F=np.eye(2), H=np.eye(2), Q=np.array([
#     [1.0, 0],
#     [0, 0.001]
# ]))

# kalman2 = KalmanFilter(F=np.eye(2), H=np.eye(2), Q=np.array([
#     [1.0, 0],
#     [0, 0.05]
# ]), x0=np.array([[0.0], [0.17]]))

# kt, ky = [], []
# for t, y in zip(t1, y1):
#     z = np.array([[t], [y]], np.float32)
#     kalman1.update(z)
#     pred = kalman1.predict()
#     kt.append(pred[0, 0])
#     ky.append(pred[1, 0])
# ax1.plot(kt, ky, color='green')

# kt, ky = [], []
# for t, y in zip(t2, y2):
#     z = np.array([[t], [y]], np.float32)
#     kalman2.update(z)
#     pred = kalman2.predict()
#     kt.append(pred[0, 0])
#     ky.append(pred[1, 0])
# ax2.plot(kt, ky, color='green')

plt.show()
