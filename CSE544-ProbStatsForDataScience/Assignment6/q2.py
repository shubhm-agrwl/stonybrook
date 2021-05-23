import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy.stats as stats


print()
print("sigma 3")
print()

df = pd.read_table("q2_sigma3.dat", sep=",", header=None)

std = 3
var = std**2

a = 0
b2 =1

para = []

std = 3
var = std**2

a = 0
b2 =1

para = []

for i in range(df.shape[0]):

  xi = df.iloc[i,:].values
  x_mean = np.mean(xi)
  n = len(xi)
  se2 = var/n
  x = (b2*x_mean + se2*a)/(b2 + se2)
  y2 = (b2*se2)/(b2 + se2)
  para.append([x,y2])
  b2 = y2
  a = x



plt.figure(figsize=(10,10))
plt.title("Sigma 3")
for i,(mu,v) in enumerate(para):
  sigma = np.sqrt(v)
  x = np.linspace(mu - 3*sigma, mu + 3*sigma, 100)
  plt.plot(x, stats.norm.pdf(x, mu, sigma), label=f"{i+1}")
plt.legend()
plt.show()

para = pd.DataFrame(para, columns = ["x","y_squared"])
print(para)


print("**********************************************************")

print()
print("sigma 100")
print()


df = pd.read_table("q2_sigma100.dat", sep=",", header=None)

std = 100
var = std**2

a = 0
b2 =1

para = []

for i in range(df.shape[0]):

  xi = df.iloc[i,:].values
  x_mean = np.mean(xi)
  n = len(xi)
  se2 = var/n
  x = (b2*x_mean + se2*a)/(b2 + se2)
  y2 = (b2*se2)/(b2 + se2)
  para.append([x,y2])
  b2 = y2
  a = x

plt.figure(figsize=(10,10))
plt.title("Sigma 100")
for i,(mu,v) in enumerate(para):
  sigma = np.sqrt(v)
  x = np.linspace(mu - 3*sigma, mu + 3*sigma, 100)
  plt.plot(x, stats.norm.pdf(x, mu, sigma), label=f"{i+1} iteration")
plt.legend()
plt.show()

para = pd.DataFrame(para, columns = ["x","y_squared"])
print(para)
