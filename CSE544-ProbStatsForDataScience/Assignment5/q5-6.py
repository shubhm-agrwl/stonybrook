import numpy as np
import pandas as pd
from scipy.stats import t
from scipy.stats import norm


X1 = pd.read_csv('q6_X1.csv')
Y1 = pd.read_csv('q6_Y1.csv')
X1 = X1.values.flatten()
Y1 = Y1.values.flatten()
n = len(X1)

print("----------------------------------")
diff = np.mean(X1) - np.mean(Y1)
varX1True = 1
varY1True = 1

print("Mean value of X1 :",np.mean(X1) )
print("Mean value of Y1 :",np.mean(Y1) )

std = np.sqrt( varX1True /len(X1) + varY1True /len(Y1))
print ("H0 : X and Y have the same mean value")

print("Std Deviation value :",std)
print ("Using Z test, we get: ")
Z = np.absolute(diff/std)
print ("Value of Z :", Z)
threshold = 1.962
if Z > threshold:
  print("Reject H0")
else:
  print("Accept H0")


p_value =  2*(1-norm.cdf(Z))
print ("P Value :", p_value)

print("----------------------------------")

X1_mean = np.mean(X1)
Y1_mean = np.mean(Y1)
diff = X1_mean - Y1_mean
var_X1 = np.sum(np.square(X1 - X1_mean))/(len(X1)-1)
var_Y1 = np.sum(np.square(Y1 - Y1_mean))/(len(Y1)-1)


std = np.sqrt( var_X1 /len(X1) + var_Y1 /len(Y1))
print ("H0 : X and Y have the same mean value")
print("Std Deviation value :",std)
print ("Using T test, we get: ")
T = np.absolute(diff/std)
print ("Value of T :", T)
threshold = 2.086
if T > threshold:
  print("Reject H0")
else:
  print("Accept H0")


p_value =  2*(1-t.cdf(T, n-1))
print ("P Value :" , p_value)

print("----------------------------------")
print("----------------------------------")
X1 = pd.read_csv('q6_X2.csv')
Y1 = pd.read_csv('q6_Y2.csv')
X1 = X1.values.flatten()
Y1 = Y1.values.flatten()
n = len(X1)
print("Mean value of X1 :",np.mean(X1))
print("Mean value of Y1 :",np.mean(Y1))

diff = np.mean(X1) - np.mean(Y1)
varX1True = 1
varY1True = 1

std = np.sqrt( varX1True /len(X1) + varY1True /len(Y1))
print ("H0 : X and Y have the same mean value")
print("Std Deviation value :",std)
print ("Using Z test, we get: ")
Z = np.absolute(diff/std)
print ("Value of Z :", Z)
threshold = 1.962
if Z > threshold:
  print("Reject H0")
else:
  print("Accept H0")


p_value =  2*(1-norm.cdf(Z))
print ("P Value :" , p_value)

print("----------------------------------")
X1_mean = np.mean(X1)
Y1_mean = np.mean(Y1)
diff = X1_mean - Y1_mean
var_X1 = np.sum(np.square(X1 - X1_mean))/(len(X1)-1)
var_Y1 = np.sum(np.square(Y1 - Y1_mean))/(len(Y1)-1)


std = np.sqrt( var_X1 /len(X1) + var_Y1 /len(Y1))
print ("H0 : X and Y have the same mean value")

print("Std Deviation value :",std)
print ("Using T test, we get: ")
T = np.absolute(diff/std)
print ("Value of T :", T)
threshold = 1.962
if T > threshold:
  print("Reject H0")
else:
  print("Accept H0")

p_value =  2*(1-t.cdf(T, n-1))
print ("P Value :" , p_value)

