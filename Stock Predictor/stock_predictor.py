import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, LSTM, Dropout
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import mean_squared_error, mean_absolute_percentage_error
from statsmodels.tsa.api import SimpleExpSmoothing

stock_data = pd.read_csv('Netflix_Dataset.csv', index_col='Date')

target_y = stock_data['Close']
X_feat = stock_data.iloc[:, 0:3]
stock_data = stock_data[['Open', 'High', 'Low', 'Close']]

sc = StandardScaler()
stock_data_ft = sc.fit_transform(stock_data.values)
stock_data_ft = pd.DataFrame(columns=stock_data.columns, data=stock_data_ft, index=stock_data.index)

def lstm_split(data, n_steps):
    X, y = [], []
    for i in range(len(data) - n_steps):
        X.append(data[i:i + n_steps, :-1])
        y.append(data[i + n_steps - 1, -1])
    return np.array(X), np.array(y)

n_steps = 10
X1, y1 = lstm_split(stock_data_ft.values, n_steps=n_steps)

train_split = 0.8
split_idx = int(np.ceil(len(X1) * train_split))
date_index = stock_data_ft.index

X_train, X_test = X1[:split_idx], X1[split_idx:]
y_train, y_test = y1[:split_idx], y1[split_idx:]
X_train_date, X_test_date = date_index[:split_idx], date_index[split_idx:-n_steps]

lstm = Sequential()
lstm.add(LSTM(50, input_shape=(X_train.shape[1], X_train.shape[2]), activation='relu', return_sequences=True))
lstm.add(LSTM(50, activation='relu', return_sequences=True))
lstm.add(LSTM(50, activation='relu'))
lstm.add(Dense(1))

lstm.compile(loss='mean_squared_error', optimizer='adam')

history = lstm.fit(X_train, y_train, epochs=100, batch_size=4, validation_split=0.2, verbose=2, shuffle=False)

y_pred = lstm.predict(X_test)
y_test_flat = y_test.squeeze()
y_pred_flat = y_pred.squeeze()

# 1. Create dummy arrays matching the original 4-column shape
dummy_test = np.zeros((len(y_test_flat), 4))
dummy_pred = np.zeros((len(y_pred_flat), 4))

# 2. Insert the scaled Close prices into the 4th column (index 3)
dummy_test[:, 3] = y_test_flat
dummy_pred[:, 3] = y_pred_flat

# 3. Apply inverse_transform and extract just the unscaled Close column
y_test_inv = sc.inverse_transform(dummy_test)[:, 3]
y_pred_inv = sc.inverse_transform(dummy_pred)[:, 3]

# 4. Calculate RMSE and MAPE on the true, unscaled dollar values
mse = mean_squared_error(y_test_inv, y_pred_inv)
rmse = np.sqrt(mse)
mape = mean_absolute_percentage_error(y_test_inv, y_pred_inv)

print("LSTM RMSE: ", rmse)
print("LSTM MAPE: ", mape)

train = stock_data[['Close']].iloc[:split_idx]
test = stock_data[['Close']].iloc[split_idx:]
test_pred_sma = np.array([train.rolling(10).mean().iloc[-1]] * len(test)).reshape((-1, 1))

print('SMA Test RMSE: %.3f' % np.sqrt(mean_squared_error(test, test_pred_sma)))
print('SMA Test MAPE: %.3f' % mean_absolute_percentage_error(test, test_pred_sma))

X_val = stock_data[['Close']].values
train_ema = X_val[:split_idx]
test_ema = X_val[split_idx:]
test_concat = np.array([]).reshape((0, 1))

for i in range(len(test_ema)):
    train_fit = np.concatenate((train_ema, np.asarray(test_concat)))
    # Add initialization_method and optimized=False here
    model = SimpleExpSmoothing(np.asarray(train_fit), initialization_method="heuristic")
    fit = model.fit(smoothing_level=0.1, optimized=False)
    test_pred_ema = fit.forecast(1)
    test_concat = np.concatenate((np.asarray(test_concat), test_pred_ema.reshape((-1, 1))))

print('EMA Test RMSE: %.3f' % np.sqrt(mean_squared_error(test_ema, test_concat)))
print('EMA Test MAPE: %.3f' % mean_absolute_percentage_error(test_ema, test_concat))