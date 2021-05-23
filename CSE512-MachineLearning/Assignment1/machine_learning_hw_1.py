import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.stats import norm
from scipy.stats import gaussian_kde
from sklearn.linear_model import LinearRegression
from sklearn import metrics
import seaborn as sns


def learn_reg_params(x, y):
    # print(np.atleast_2d(x).T)
    # print(np.atleast_2d(y).T)

    x = np.atleast_2d(x).T
    y = np.atleast_2d(y).T

    y_actual = y

    for i in range(7):
        x = np.insert(x, i + 1, 0, axis=1)
        y = np.insert(y, i + 1, 0, axis=1)
        x[i + 1:, i + 1] = x[:-(i + 1), 0]
        y[i + 1:, i + 1] = y[:-(i + 1), 0]

    # print(label_y)

    data = np.append(x[:, 1:], y[:, 1:], 1)
    # X = x[:, 1:]
    # print(X)

    linear_regression = LinearRegression()
    linear_regression.fit(data[8:, :], y_actual[8:])

    print("coefficients: ", linear_regression.coef_)
    print("intercept: ", linear_regression.intercept_)

    # print(linear_regression.predict(X))

    # x_test = [[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
    #           [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2],
    #           [10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10],
    #           [20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20],
    #           [99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99]]

    y_predicted = linear_regression.predict(data[8:, :])
    # y_pred = linear_regression.predict(x_test)
    x_coordinates = np.arange(220)

    # print(x_coordinates.shape)
    # print(y_actual.shape)
    # print(y_predicted.shape)

    # x_test =

    plt.figure(1)
    plt.scatter(x_coordinates, y_actual, color='green')
    plt.scatter(x_coordinates[:212] + 8, y_predicted, color='blue')
    # print(x[:, 0].shape)
    # plt.scatter(X[8:, 0], y_pred, color='blue')

    mae = metrics.mean_absolute_error(y_actual[8:], y_predicted)
    mse = metrics.mean_squared_error(y_actual[8:], y_predicted)
    rmse = np.sqrt(metrics.mean_squared_error(y_actual[8:], y_predicted))

    # print("MAE", mae)
    # print("MSE", mse)
    # print("RMSE", rmse)

    plt.figure(2)

    mu = mae
    sd = rmse
    x1 = -1000
    x2 = 1000

    x = np.linspace(x1, x2, 100)
    y = norm.pdf(x, mu, sd)

    plt.plot(x, y, color='blue')
    plt.grid()

    plt.xlim(x1, x2)
    plt.ylim(0, 0.005)

    # np.histogram(y_actual[8:] - y_predicted)
    # print(y_actual[8:] - y_predicted)

    # plt.figure(3)

    diff = y_actual[8:] - y_predicted
    # n, bins, patches = plt.hist(x=y_actual[8:] - y_predicted, bins='auto', color='#0504aa',
    #                             alpha=0.7, rwidth=0.85)
    # plt.grid(axis='y', alpha=0.75)
    # plt.xlabel('Value')
    # plt.ylabel('Frequency')
    # plt.title('My Very Own Histogram')
    # plt.text(23, 45, r'$\mu=15, b=3$')
    # maxfreq = n.max()
    # # Set a clean upper y-axis limit.
    # plt.ylim(ymax=np.ceil(maxfreq / 10) * 10 if maxfreq % 10 else maxfreq + 10)

    # kde = gaussian_kde(y_actual[8:] - y_predicted)
    # pdf = kde.evaluate(np.arange(start=-1000, stop=1000))
    # plt.plot(np.arange(start=-1000, stop=1000), pdf)

    # sns.distplot(diff, hist=False, rug=True,
    #              # axlabel="Something ?",
    #              kde_kws=dict(label="kde"),
    #              # rug_kws=dict(height=.2, linewidth=2, color="C1", label="data")
    #              )
    # plt.legend()

    # KDE instance
    # bw = 1. / np.std(diff)
    # g_kde = gaussian_kde(dataset=diff, bw_method="scott")

    # compute KDE
    # gridsize = 200
    # g_x = np.linspace(-24, 6, gridsize)
    # g_kde_values = g_kde(g_x)

    # plt.plot(g_x, g_kde_values, label="scipy")
    # plt.plot(x, kde, "o", label="by hands")
    # plt.legend();

    ## WORKING

    sns.set_style("white")

    # Plot
    kwargs = dict(hist_kws={'alpha': .6}, kde_kws={'linewidth': 2, 'shade': True})

    # plt.figure(figsize=(10, 7), dpi=80)
    sns.distplot(diff, color="dodgerblue", **kwargs, hist=False)
    plt.xlim(-750, 750)

    plt.figure(3)
    plt.hist(diff, range=(-750, 750), bins=25)

    plt.show()

    return x, y


def main():
    df = pd.read_csv('covid19_time_series.csv', ',')
    data = df.to_numpy()
    print(data.shape)
    data = data[:, 1:]
    data = np.transpose(data)

    learn_reg_params(data[:, 0], data[:, 1])

    # for i in range(20):
    #     print(data[i])
    # data = data.transpose()
    # print(data.head())
    #
    # data.columns = ['confirmed', 'deaths']
    # x = data[0]
    # y = data[1]
    #
    # print(x)
    # print(y)

    # data = np.genfromtxt('covid19_time_series.csv', delimiter=',', dtype=None)
    # print(data)


if __name__ == '__main__':
    main()
