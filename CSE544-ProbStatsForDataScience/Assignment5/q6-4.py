import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('data_q4_1.csv')

D1 = df['avg_glucose_level'][df.stroke == 1].values
D2 = df['avg_glucose_level'][df.stroke == 0].values

mean_D1 = np.mean(D1)
print("The mean of data where patient gets stroke : ", mean_D1)
mean_D2 = np.mean(D2)
print("The mean of data where patient does not get stroke : ", mean_D2)

Tobs = np.absolute(mean_D1 - mean_D2)

n_D1 = len(D1)
n_D2 = len(D2)


def runPermTest(iter, df, n_D1, column):
    table = {'T0': []}
    for i in range(iter):
        permuted_df = np.random.permutation(df)
        p_D1 = permuted_df[:n_D1, column]
        p_D2 = permuted_df[n_D1:, column]
        p_mean_D1 = np.mean(p_D1)
        p_mean_D2 = np.mean(p_D2)
        T0 = np.absolute(p_mean_D1 - p_mean_D2)
        table['T0'].append(T0)

    table_df = pd.DataFrame.from_dict(table)
    p_value = np.sum(table_df.values.flatten() > Tobs) / len(table_df)
    print("Running Permutation test for " + str(iter) + " iterations")
    print("The p value is :", p_value)
    if p_value <= 0.05:
        print(f"Since p_value {p_value}  is less than 0.05 threshold, we reject H0")
    else:
        print((f"Since p_value {p_value} is greater than 0.05 threshold, we accept H0"))


print("--------------a)--------------------")

print(
    "The H0 Hypothesis is : People getting stroke tend to have the same glucose level as people who do not get stroke")

runPermTest(200, df, n_D1, 0)

runPermTest(1000, df, n_D1, 0)

print("--------------b)--------------------")

df = pd.read_csv('data_q4_2.csv')

df.head()

D1 = df['age'][df.gender == 'Male'].values
D2 = df['age'][df.gender == 'Female'].values

mean_D1 = np.mean(D1)
print("The mean of age of Male patients: ", mean_D1)
mean_D2 = np.mean(D2)
print("The mean of age of Female patients : ", mean_D2)

Tobs = np.absolute(mean_D1 - mean_D2)

n_D1 = len(D1)
n_D2 = len(D2)

print("The H0 Hypothesis is : female patients get a stroke at the same age as male patients")

runPermTest(1000, df, n_D1, 1)

print("-----------------------------------")

print("PART C")


def get_xy(x):
    n = len(x)
    x = sorted(x)
    x_cdf = []
    y_cdf = []
    y_curr = 0

    x_cdf.append(0)
    y_cdf.append(0)

    for i in x:
        y_curr += 1 / n
        y_cdf.append(y_curr)
        x_cdf.append(i)

    return x_cdf, y_cdf


def draw_ecdf(x1, y1, x2, y2, max_diff, xdiff):
    plt.figure(figsize=(20, 10))
    plt.step(x1, y1, where="post", label="CDF-D1")
    plt.step(x2, y2, where="post", label="CDF-D2")
    plt.xticks(x1 + x2, rotation=90)
    plt.yticks(np.arange(0, 1.1, 1 / 10))
    plt.title(f"Empirical CDF |KS Statistic | Max Difference {max_diff} at {xdiff}")
    plt.xlabel("Sample Points")
    plt.ylabel("Pr[X<x]")
    plt.scatter(x1 + x2, [0] * len(x1 + x2), color='red', marker='x', s=100, label='Change Points')
    plt.scatter([xdiff], [0], color='blue', marker='o', s=200, label=f'Max Diff {max_diff}')
    plt.grid(which="both")
    plt.legend(loc="upper left")
    plt.show()


x1, y1 = get_xy(D1)
x2, y2 = get_xy(D2)

X = np.unique(x1 + x2)

y1_all = []
temp = 0
for i in np.arange(100):
    ind = np.where(np.array(x1) == i)[0]
    if len(ind) == 0:
        y1_all.append(temp)
    else:
        y1_all.append(y1[ind[-1]])
        temp = y1[ind[-1]]

y2_all = []
temp = 0
for i in np.arange(100):
    ind = np.where(np.array(x2) == i)[0]
    if len(ind) == 0:
        y2_all.append(temp)
    else:
        y2_all.append(y2[ind[-1]])
        temp = y2[ind[-1]]

diff = []
for i in np.arange(100):
    diff.append(np.absolute(y1_all[i] - y2_all[i]))

max_diff = np.max(diff)

max_diff_arg = np.argmax(diff)
draw_ecdf(x1, y1, x2, y2, max_diff, max_diff_arg)

print("----------------------------------")


