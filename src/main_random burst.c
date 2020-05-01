#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc32-c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include"epanet2.h"

#define EMITTER_MIN_VALUE 1
#define EMITTER_MAX_VALUE 50
#define TIME_INDEX_MIN 0
#define TIME_INDEX_MAX 119
#define PUMP_NUM 3
#define BURST_NUM 6
#define BURST_PRESSURE_FILE_NAME "burst_pressure.csv"
#define BURST_FLOW_FILE_NAME "burst_flow.csv"

void openepanet(char *f1, char *f2, char *f3);
int getRandomInt(int min, int max);
void printFloatArray(float arr[], int s, int e, char str[]);
void randomBurst(float pressure[], float flow[], int timeIndexes[], int i);
void writeFloatMatrixToFile(const float *arr, int rows, int cols, char *filename);
void writeIntMatrixToFile(const int *arr, int rows, int cols, char *filename);
// void rulePressure();

int Nall;
int Ntank;
int Nlink;
int Nnode;
int Duration;

int main() {
    openepanet("anytown.inp", "sample.rtp", "");//打开工具箱

    ENgetcount(EN_NODECOUNT, &Nall);
    ENgetcount(EN_TANKCOUNT, &Ntank);
    ENgetcount(EN_LINKCOUNT, &Nlink);
    long secondDuration;
    ENgettimeparam(EN_DURATION, &secondDuration);
    Nnode = Nall - Ntank;
    Nlink -= PUMP_NUM;
    Duration = secondDuration / 3600;
    printf("Nall = %d, Ntank = %d, Nnode = %d, Nlink = %d, Duration = %d\n", Nall, Ntank, Nnode, Nlink, Duration);

    // 长度为Nnode+1的float数组，保存初始的扩散器系数，用于在每次随机爆管事件之前进行所有节点扩散器系数的还原
    float initEmitters[Nnode + 1];
    // 将initEmitters数组的所有元素初始化为0
    memset(initEmitters, 0, sizeof(initEmitters));
    for (int i = 1; i <= Nnode; i++) {
        // 获取第i个节点的扩散器系数，保存到ininEmitters[i]中
        ENgetnodevalue(i, EN_EMITTER, initEmitters + i);
    }

    float pressureBaseline[Duration][Nnode];
    float flowBaseline[Duration][Nlink];
    memset(pressureBaseline, 0, sizeof(pressureBaseline));
    memset(flowBaseline, 0, sizeof(flowBaseline));

    int count = 0;
    long t, tstep;
    ENopenH();
    ENinitH(0);
    do
    {
        ENrunH(&t);
        if (t % 3600 == 0) {
            for (int i = 0; i < Nnode; i++) {
                ENgetnodevalue(i + 1, EN_PRESSURE, &pressureBaseline[count][i]);
            }
            for (int i = 0; i < Nlink; i++) {
                ENgetlinkvalue(i + 1, EN_FLOW, &flowBaseline[count][i]);
            }
            count++;
        }
        ENnextH(&tstep);
    } while (tstep > 0);
    ENcloseH();

    writeFloatMatrixToFile(*pressureBaseline, Duration, Nnode, "pressureBaseline.csv");
    writeFloatMatrixToFile(*flowBaseline, Duration, Nlink, "flowBaseline.csv");

    // 保存所有随机爆管事件的压力数据，100 * 22的矩阵，从下标0开始
    float pressures[BURST_NUM][Nnode];
    // 保存所有随机爆管事件的流量数据，100 * 43的矩阵，从下标0开始
    float flows[BURST_NUM][Nlink];
    // 保存每次随机保管时间选取的随机时间节点
    int timeIndexes[BURST_NUM];
    memset(timeIndexes, 0, sizeof(timeIndexes));
    for (int i = 0; i < BURST_NUM; i++) {

        // 在每次随机爆管事件发生前，还原所有节点的扩散器系数
        for (int j = 1; j <= Nnode; j++) {
            ENsetnodevalue(j, EN_EMITTER, initEmitters[j]);
        }

        printf("the %dth time random burst start: \n", i + 1);

        // 保存每次随机爆管事件产生的压力数据
        float pressure[Nnode + 1];
        // 保存每次随机爆管事件产生的流量数据
        float flow[Nlink + 1];
        memset(pressure, 0, sizeof(pressure));
        memset(flow, 0, sizeof(flow));

        // 发生随机爆管事件
        randomBurst(pressure, flow, timeIndexes, i);

        // 将本次随机爆管事件产生的压力数据保存到矩阵的第i行中
        for (int j = 1; j <= Nnode; j++) {
            pressures[i][j - 1] = pressure[j];
        }
        // 将本次随机爆管事件产生的流量数据保存到矩阵的第i行中
        for (int j = 1; j <= Nlink; j++) {
            flows[i][j - 1] = flow[j];
        }
        printf("the %dth time random burst end!\n\n", i + 1);
    }
    ENclose();


    writeFloatMatrixToFile(*pressures, BURST_NUM, Nnode, BURST_PRESSURE_FILE_NAME);
    writeFloatMatrixToFile(*flows, BURST_NUM, Nlink, BURST_FLOW_FILE_NAME);
    writeIntMatrixToFile(timeIndexes, BURST_NUM, 1, "randomTimeIndexes.csv");



    return 0;
}

void randomBurst(float pressure[], float flow[], int timeIndexes[], int i) {

    // 选取随机节点
    int randomNode = getRandomInt(1, Nnode);
    printf("randomNode = %d\n", randomNode);
    // 选取随机的扩散器系数
    float value = (float) getRandomInt(EMITTER_MIN_VALUE, EMITTER_MAX_VALUE);
    // 设置节点的扩散器系数
    ENsetnodevalue(randomNode, EN_EMITTER, value);
    printf("set the emitter of node %d as %.2f\n", randomNode, value);

    // 选取随机时间节点
    int randomTimeIndex = getRandomInt(TIME_INDEX_MIN, TIME_INDEX_MAX);
    // 将本次事件选取观测的随机时间节点保存至timeIndexes数组中，该数组在main函数中定义
    timeIndexes[i] = randomTimeIndex;
    printf("the selected random time step: %d\n", randomTimeIndex);

    long t, tstep;
    ENopenH();
    ENinitH(0);
    do
    {
        ENrunH(&t);
        // 当当前时刻等于选取的时间节点时，观测每个节点的压力和每个管段的流量
        if (t == 3600 * randomTimeIndex) {
            // 观测每个节点的压力，将结果保存到pressure数组，从下标1开始
            for (int index = 1; index <= Nnode; index++) {
                ENgetnodevalue(index, EN_PRESSURE, pressure + index);
            }
            // 观测每个管段的流量，flow，从下标1开始
            for (int index = 1; index <= Nlink; index++) {
                ENgetlinkvalue(index, EN_FLOW, flow + index);
            }
            // 观测后直接退出，不再进行接下来水力分析
            break;
        }
        /*
        检索时刻t的水力结果
        */
        ENnextH(&tstep);
    } while (tstep > 0);
    ENcloseH();
    printFloatArray(pressure, 1, Nnode, "pressure: ");
    printFloatArray(flow, 1, Nlink, "flow: ");
}

void openepanet(char *f1, char *f2, char *f3)  //打开epanet文件
{
    int errcode;
    errcode = ENopen(f1, f2, f3);
    //printf("the errcode is %d\n",errcode);
    if (errcode > 0)
    {
        printf("open the file is error.\n");
        ENclose();
        getchar();
        exit(1);
    }
    else
    {
        printf("open the file is successful!\n");

    }

}

int getRandomInt(int min, int max) {
    Sleep(1000);
    srand((unsigned int)time(NULL));
    int x = rand() % (max - min + 1) + min;
    return x;
}

//void sampleArray(int index[], int sampleNum, int Nnode) {
//    int flag[Nnode + 1];
//    memset(flag, 0, sizeof(index));
//    int count = 0;
//    while (count < sampleNum) {
//        int x = getRandomInt(1, Nnode);
//        if (flag[x] == 0) {
//            flag[x] = 1;
//            index[count++] = x;
//        }
//    }
//    qsort(index, sampleNum, sizeof(index[0]), cmp);
//}

//int cmp(const void *a, const void *b) {
//    return (*(int *)a - *(int *)b);
//}

void printFloatArray(float arr[], int s, int e, char str[]) {
    printf("%s", str);
    for (int i = s; i <= e; i++) {
        if (i == s) {
            printf("[%.2f", arr[i]);
        } else {
            printf(", %.2f", arr[i]);
        }
    }
    printf("]\n");
}

void writeFloatMatrixToFile(const float *arr, int rows, int cols, char *filename) {
    FILE *fp = NULL;
    fp = fopen(filename, "w+");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        exit(2);
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(fp, j == 0 ? "%f" : ",%f", *(arr + cols * i + j));
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void writeIntMatrixToFile(const int *arr, int rows, int cols, char *filename) {
    FILE *fp = NULL;
    fp = fopen(filename, "w+");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        exit(2);
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(fp, j == 0 ? "%d" : ",%d", *(arr + cols * i + j));
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}