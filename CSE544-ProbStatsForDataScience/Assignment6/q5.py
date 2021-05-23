import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


df = pd.read_table("q5.csv", sep=',')


print("Using Full Dataset")
print()

X = df.iloc[:,1:].values
y = df.iloc[:,0].values

X_train, X_test = X[:400], X[400:]
y_train, y_test = y[:400], y[400:]

y_train = y_train.reshape((-1,1))
y_test = y_test.reshape((-1,1))


# X_train = np.append( np.ones((len(X_train),1)), X_train, axis=1)
# X_test = np.append( np.ones((len(X_test),1)), X_test, axis=1)

Beta = np.matmul( np.linalg.inv(np.matmul(X_train.T,  X_train)), np.matmul(X_train.T, y_train ))

print("Beta")
print(Beta)
print()

test_sse = np.sum(np.square( y_test - np.matmul(X_test, Beta) ))
print(f"Test SSE: {test_sse}")
print()



print("Using TOEFL, SOP, LOR")

df_new = df.loc[:,['Chance of Admit','TOEFL Score','SOP','LOR']]
X = df_new.iloc[:,1:].values
y = df_new.iloc[:,0].values

X_train, X_test = X[:400], X[400:]
y_train, y_test = y[:400], y[400:]

y_train = y_train.reshape((-1,1))
y_test = y_test.reshape((-1,1))

# X_train = np.append( np.ones((len(X_train),1)), X_train, axis=1)
# X_test = np.append( np.ones((len(X_test),1)), X_test, axis=1)
Beta = np.matmul( np.linalg.inv(np.matmul(X_train.T,  X_train)), np.matmul(X_train.T, y_train ))

print()
print("Beta")
print(Beta)
print()

test_sse = np.sum(np.square( y_test - np.matmul(X_test, Beta) ))
print(f"Test SSE: {test_sse}")
print()



print("Using GRE, GPA")


df_new = df.loc[:,["Chance of Admit",'GRE Score','GPA']]
X = df_new.iloc[:,1:].values
y = df_new.iloc[:,0].values

X_train, X_test = X[:400], X[400:]
y_train, y_test = y[:400], y[400:]

y_train = y_train.reshape((-1,1))
y_test = y_test.reshape((-1,1))

# X_train = np.append( np.ones((len(X_train),1)), X_train, axis=1)
# X_test = np.append( np.ones((len(X_test),1)), X_test, axis=1)
Beta = np.matmul( np.linalg.inv(np.matmul(X_train.T,  X_train)), np.matmul(X_train.T, y_train ))

print()
print("Beta")
print(Beta)
print()

test_sse = np.sum(np.square( y_test - np.matmul(X_test, Beta) ))
print(f"Test SSE: {test_sse}")
print()






