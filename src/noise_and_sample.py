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


def get_white_noise(mu=0, sigma=0.5, size=360):
    _white_noise = np.random.normal(mu, sigma, size)
    plt.hist(_white_noise, 50)
    # plt.show()
    return _white_noise


# 分节点去除重复区间，爆管时间段不跨越天
# x[0]:day, x[1]:time, x[2]:len, x[3]:emitter, x[4]:node index
def get_burst_sample(n):
    res = random.sample(range(0, 288 * 180 * 126), math.ceil(n * 1.5))
    # res = [[(x % (288 * 180)) // 288, (x % (288 * 180)) % 288, random.randint(1, 60), x // (288 * 180)] for x in res]
    res = [[(x // 126) // 288 + 180, (x // 126) % 288, random.randint(1, 60), random.randint(20, 101), x % 126] for x in
           res]
    res = list(filter(lambda x: x[1] + x[2] < 288, res))
    res.sort(key=lambda x: x[0] * 288 + x[1])
    groups = {}
    for x in res:
        if x[-1] not in groups:
            groups[x[-1]] = []
        groups[x[-1]].append(x)
    res = []
    for node, l in groups.items():
        res.append(l[0])
        for cur in l:
            pre = res[-1]
            if pre[0] * 288 + pre[1] + pre[2] < cur[0] * 288 + cur[1]:
                res.append(cur)
    res = random.sample(res, n)
    res.sort(key=lambda x: x[0] * 288 + x[1])
    return res


# 一起去除重复区间（同一个时刻只可能有一个节点爆管），爆管时间段不跨天
# x[0]:day, x[1]:time, x[2]:len, x[3]:emitter, x[4]:node index
def get_burst_sample2(n):
    res = random.sample(range(0, 288 * 180), math.ceil(n * 1.5))
    # res = [[(x % (288 * 180)) // 288, (x % (288 * 180)) % 288, random.randint(1, 60), x // (288 * 180)] for x in res]
    res = [[x // 288 + 180, x % 288, random.randint(1, 60), random.randint(20, 101), random.randint(1, 127)] for x in
           res]
    res = list(filter(lambda x: x[1] + x[2] < 288, res))
    res.sort(key=lambda x: x[0] * 288 + x[1])
    temp_res = [res[0]]
    for i in range(1, len(res)):
        pre = temp_res[-1]
        cur = res[i]
        if pre[0] * 288 + pre[1] + pre[2] < cur[0] * 288 + cur[1]:
            temp_res.append(cur)
    res = random.sample(temp_res, n)
    res.sort(key=lambda x: x[0] * 288 + x[1])
    return res


if __name__ == '__main__':
    random.seed(324)
    np.random.seed(324)

    time_list = get_time_list('2018-01-01 00:00:00', '2018-12-26 23:55:00', 5)
    white_noise = get_white_noise(0, 0.5, 360)
    burst_sample = get_burst_sample2(200)

    # pd.DataFrame(time_list, columns=['datetime']).to_csv('../bin/time_list.csv', index=False, header=False)
    pd.DataFrame(white_noise, columns=['noise']).to_csv('../bin/white_noise.csv', index=False, header=False)
    pd.DataFrame(burst_sample, columns=['day', 'start', 'interval', 'emitter', 'node']).to_csv(
        '../bin/burst_sample.csv', index=False, header=False)
