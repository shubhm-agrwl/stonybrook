import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


df = pd.read_table("q6.csv", sep=',', header=None)

H0 = np.array([0.1,0.3,0.5,0.8])

def MAP_decision(w, p):
  mean = 0.5
  var = 1
  result = []
  res = np.log(p/(1-p)) * (var/(2 * mean))
  if np.sum(w) <= res:
    return 0
  else:
    return 1


def run(p, df):
  arr = []
  for i in range(df.shape[1]):
    w = df.iloc[:,i]
    arr.append(MAP_decision(w,p))
  return arr


for i in range(len(H0)):
  arr = run(H0[i], df)
  s = ""
  for a in arr:
    s = s+ str(a) +" "
  print('For p(H0) = ' + str(H0[i]) + ', the hypotheses selected are :: ', s)

