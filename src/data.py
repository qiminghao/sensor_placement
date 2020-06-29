from datetime import datetime
from datetime import timedelta
import random
import math
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def get_time_list(_min_time, _max_time, time_step_minutes):
    time_delta = timedelta(minutes=time_step_minutes)
    time = datetime.strptime(_min_time, "%Y-%m-%d %H:%M:%S") - time_delta
    _time_list = []
    while True:
        time += time_delta
        #         time_str = time.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        time_str = time.strftime("%Y-%m-%d %H:%M:%S")
        _time_list.append(time_str)
        if time_str == _max_time:
            break
    return _time_list


if __name__ == '__main__':
    random.seed(324)
    np.random.seed(324)

    date_time = get_time_list('2018-01-01 00:00:00', '2018-12-26 23:55:00', 5)
    date = list(map(lambda x: x.split(' ')[0], date_time))
    time = list(map(lambda x: x.split(' ')[1], date_time))
    data = pd.read_csv('../bin/base.csv', header=None, names=['pressure52', 'pressure64', 'pressure116', 'pressure125'])
    # data.columns = ['pressure52', 'pressure64', 'pressure116', 'pressure125']
    pressure52 = list(data['pressure52'])
    pressure64 = list(data['pressure64'])
    pressure116 = list(data['pressure116'])
    pressure125 = list(data['pressure125'])
    df = pd.DataFrame(
        {'datetime': date_time, 'date': date, 'time': time, 'pressure52': pressure52, 'pressure64': pressure64,
         'pressure116': pressure116, 'pressure125': pressure125})
    df.to_csv('../bin/data.csv', index=False)
