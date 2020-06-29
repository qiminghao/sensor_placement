#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc32-c"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include"epanet2.h"

#define BASE_DEMAND_NUM 200
#define PI 3.1415926f
#define EMITTER_MIN_VALUE 1
#define EMITTER_MAX_VALUE 50
#define TIME_INDEX_MIN 0
#define TIME_INDEX_MAX 959
#define PUMP_NUM 3
#define BURST_NUM 100
#define BURST_PRESSURE_FILE_NAME "burst_pressure.csv"
#define BURST_FLOW_FILE_NAME "burst_flow.csv"
#define INP_FILE_NAME "epanet/Austin net modified(24h).inp"
#define MAX_SENSOR_NUM 4

void openepanet(char *f1, char *f2, char *f3);

void getEpanetInfo();

int getRandomInt(int min, int max);

void printFloatArray(float arr[], int s, int e, char str[]);

void randomBurst();

void randomNormal();

void writeFloatMatrixToFile(const float *arr, int rows, int cols, char *filename);

void readFloatMatrixFromFile(float **mat, int rows, int cols, char *filename);

void writeIntMatrixToFile(const int *arr, int rows, int cols, char *filename);

void gauss(float *arr, float ex, float dx, int n);

void runBaseline(char *filename, char *flowFilename);

void generateBaselineFile();

void computeExAndSigma();

void computeDetectionMatrix();

void generateRandomSensorLocation(int n);

void computeDetectionProbMatrix();

void findOptimalVectors();

// void rulePressure();

int Nall;
int Ntank;
int Nlink;
int Nnode;
int Duration;
float DP;
float RF;

int main() {
    srand((unsigned) time(NULL));

    openepanet(INP_FILE_NAME, "sample.rtp", "");//打开工具箱

//    printf("getEpanetInfo start\n");
    getEpanetInfo();

//    printf("generateBaselineFile strat\n");
//    generateBaselineFile();
//
//    printf("computeExAndSigma start\n");
//    computeExAndSigma();
//
//    printf("randomNormal start\n");
//    randomNormal();
//
//    printf("randomBurst strat\n");
//    randomBurst();
//
//    printf("computeDetectionMatrix start\n");
//    computeDetectionMatrix();
//
//    for (int i = 1; i <= MAX_SENSOR_NUM; i++) {
//        printf("%dth generateRandomSensorLocation start\n", i);
//        generateRandomSensorLocation(i);
//
//        printf("%dth computeDetectionProbMatrix start\n", i);
//        computeDetectionProbMatrix();
//
//        printf("%d findOptimalVectors start\n", i);
//        findOptimalVectors();
//    }

    ENclose();

    return 0;
}

void randomNormal() {

    // 保存所有随机爆管事件的压力数据，100 * 22的矩阵，从下标0开始
    float normalPressures[BURST_NUM][Nnode];
    // 保存所有随机爆管事件的流量数据，100 * 43的矩阵，从下标0开始
    float normalFlows[BURST_NUM][Nlink];
    // 保存每次随机保管时间选取的随机时间节点
    int timeIndexes[BURST_NUM];
    memset(normalPressures, 0, sizeof(normalPressures));
    memset(normalFlows, 0, sizeof(normalFlows));
    memset(timeIndexes, 0, sizeof(timeIndexes));

    for (int i = 0; i < BURST_NUM; i++) {
        printf("the %dth time random normal start: \n", i + 1);

        // 选取随机时间节点
        int randomTimeIndex = getRandomInt(TIME_INDEX_MIN, TIME_INDEX_MAX);
        // 将本次事件选取观测的随机时间节点保存至timeIndexes数组中，该数组在main函数中定义
        timeIndexes[i] = randomTimeIndex;
        printf("the selected random time step: %d\n", randomTimeIndex);
        long t, tstep;
        ENopenH();
        ENinitH(0);
        do {
            ENrunH(&t);
            // 当当前时刻等于选取的时间节点时，观测每个节点的压力和每个管段的流量
            if (t == 3600 * randomTimeIndex) {
                // 观测每个节点的压力，将结果保存到pressure数组，从下标1开始
                for (int index = 0; index < Nnode; index++) {
                    ENgetnodevalue(index + 1, EN_PRESSURE, &normalPressures[i][index]);
                }
                // 观测每个管段的流量，flow，从下标1开始
                for (int index = 0; index < Nlink; index++) {
                    ENgetlinkvalue(index + 1, EN_FLOW, &normalFlows[i][index]);
                }
                // 观测后直接退出，不再进行接下来水力分析
                break;
            }
            ENnextH(&tstep);
        } while (tstep > 0);
        ENcloseH();
        printf("the %dth time random burst end!\n\n", i + 1);
    }

    writeFloatMatrixToFile(*normalPressures, BURST_NUM, Nnode, "normal_pressure.csv");
    writeFloatMatrixToFile(*normalFlows, BURST_NUM, Nlink, "normal_flow.csv");
    writeIntMatrixToFile(timeIndexes, BURST_NUM, 1, "normal_timeIndexes.csv");
}

void randomBurst() {

    // 长度为Nnode+1的float数组，保存初始的扩散器系数，用于在每次随机爆管事件之前进行所有节点扩散器系数的还原
    float initEmitters[Nnode + 1];
    // 将initEmitters数组的所有元素初始化为0
    memset(initEmitters, 0, sizeof(initEmitters));
    for (int i = 1; i <= Nnode; i++) {
        // 获取第i个节点的扩散器系数，保存到ininEmitters[i]中
        ENgetnodevalue(i, EN_EMITTER, initEmitters + i);
    }

    // 保存所有随机爆管事件的压力数据，100 * 22的矩阵，从下标0开始
    float burstPressures[BURST_NUM][Nnode];
    // 保存所有随机爆管事件的流量数据，100 * 43的矩阵，从下标0开始
    float burstFlows[BURST_NUM][Nlink];
    // 保存每次随机保管时间选取的随机时间节点
    int timeIndexes[BURST_NUM];
    memset(burstPressures, 0, sizeof(burstPressures));
    memset(burstFlows, 0, sizeof(burstFlows));
    memset(timeIndexes, 0, sizeof(timeIndexes));

    for (int i = 0; i < BURST_NUM; i++)

        printf("the %dth time random burst start: \n", i + 1);

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
        do {
            ENrunH(&t);
            // 当当前时刻等于选取的时间节点时，观测每个节点的压力和每个管段的流量
            if (t == 3600 * randomTimeIndex) {
                ENsetnodevalue(randomNode, EN_EMITTER, value);
                // 观测每个节点的压力，将结果保存到pressure数组，从下标1开始
                for (int index = 0; index < Nnode; index++) {
                    ENgetnodevalue(index + 1, EN_PRESSURE, &burstPressures[i][index]);
                }
                // 观测每个管段的流量，flow，从下标1开始
                for (int index = 0; index < Nlink; index++) {
                    ENgetlinkvalue(index + 1, EN_FLOW, &burstFlows[i][index]);
                }
            }
            ENnextH(&tstep);
        } while (tstep > 0);
        ENcloseH();
        printf("the %dth time random burst end!\n\n", i + 1);

        // 在每次随机爆管事件发生后，还原所有节点的扩散器系数
        for (int j = 1; j <= Nnode; j++) {
            ENsetnodevalue(j, EN_EMITTER, initEmitters[j]);
        }
    }

    writeFloatMatrixToFile(*burstPressures, BURST_NUM, Nnode, BURST_PRESSURE_FILE_NAME);
    writeFloatMatrixToFile(*burstFlows, BURST_NUM, Nlink, BURST_FLOW_FILE_NAME);
    writeIntMatrixToFile(timeIndexes, BURST_NUM, 1, "burst_timeIndexes.csv");
}

void openepanet(char *f1, char *f2, char *f3)  //打开epanet文件
{
    int errcode;
    errcode = ENopen(f1, f2, f3);
    //printf("the errcode is %d\n",errcode);
    if (errcode > 0) {
        printf("open the epanet file is error.\n");
        ENclose();
        getchar();
        exit(1);
    } else {
        printf("open the epanet file is successful!\n");

    }

}

int getRandomInt(int min, int max) {
    int x = rand() % (max - min + 1) + min;
    return x;
}

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
        return;
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

void gauss(float *arr, float ex, float dx, int n) {

    for (int i = 0; i < n; i++) {
        float rand1 = (float) rand() / (RAND_MAX + 1);
        float rand2 = (float) rand() / (RAND_MAX + 1);
        arr[i] = (sqrtf(-2 * logf(rand1)) * cosf(rand2 * 2 * PI)) * sqrtf(dx) + ex;
        arr[i] = fabsf(arr[i]);
    }
}

void runBaseline(char *pressureFilename, char *flowFilename) {

    float pressureBaseline[Duration][Nnode];
    float flowBaseline[Duration][Nlink];
    memset(pressureBaseline, 0, sizeof(pressureBaseline));
    memset(flowBaseline, 0, sizeof(flowBaseline));

    long t, tstep;
    ENopenH();
    ENinitH(0);
    do {
        ENrunH(&t);
        if (t % 3600 == 0) {
            int index = t / 3600;
            if (index == Duration) {
                break;
            }
            for (int i = 0; i < Nnode; i++) {
                ENgetnodevalue(i + 1, EN_PRESSURE, &pressureBaseline[index][i]);
            }
            for (int i = 0; i < Nlink; i++) {
                ENgetlinkvalue(i + 1, EN_FLOW, &flowBaseline[index][i]);
            }
        }
        ENnextH(&tstep);
    } while (tstep > 0);
    ENcloseH();

    writeFloatMatrixToFile(*pressureBaseline, Duration, Nnode, pressureFilename);
    writeFloatMatrixToFile(*flowBaseline, Duration, Nlink, flowFilename);
}

void generateBaselineFile() {
    float initBaseDemand[Nnode];
    for (int i = 1; i <= Nnode; i++) {
        ENgetnodevalue(i, EN_BASEDEMAND, &initBaseDemand[i - 1]);
    }
    printFloatArray(initBaseDemand, 0, Nnode - 1, "initBaseDemand: ");

    float baseDemand[BASE_DEMAND_NUM][Nnode];
    memset(baseDemand, 0, sizeof(baseDemand));

    for (int i = 0; i < Nnode; i++) {
        float *temp;
        temp = (float *) malloc(BASE_DEMAND_NUM * sizeof(float));
        gauss(temp, initBaseDemand[i], 1, BASE_DEMAND_NUM);
        for (int j = 0; j < BASE_DEMAND_NUM; j++) {
            baseDemand[j][i] = temp[j];
        }
    }

    for (int i = 0; i < BASE_DEMAND_NUM; i++) {
        char pressureFilename[50];
        char flowFilename[50];
        sprintf(pressureFilename, "pressure_baseline/%d.csv", i + 1);
        sprintf(flowFilename, "flow_baseline/%d.csv", i + 1);

        printf("the %dth random base demand.\n", i + 1);
        printFloatArray(baseDemand[i], 0, Nnode - 1, "");
        printf("\n\n");
        for (int j = 1; j <= Nnode; j++) {
            ENsetnodevalue(i, EN_BASEDEMAND, baseDemand[i][j - 1]);
        }
        runBaseline(pressureFilename, flowFilename);
    }
    for (int i = 1; i <= Nnode; i++) {
        ENsetnodevalue(i, EN_BASEDEMAND, initBaseDemand[i - 1]);
    }
}

void computeExAndSigma() {
    float u_pressure[Duration][Nnode];
    float sigma_pressure[Duration][Nnode];
    float u_flow[Duration][Nlink];
    float sigma_flow[Duration][Nlink];
    memset(u_pressure, 0, sizeof(u_pressure));
    memset(sigma_pressure, 0, sizeof(sigma_pressure));
    memset(u_flow, 0, sizeof(u_flow));
    memset(sigma_flow, 0, sizeof(sigma_flow));

    for (int i = 0; i < BASE_DEMAND_NUM; i++) {
        char pressureFilename[50];
        char flowFilename[50];
        sprintf(pressureFilename, "pressure_baseline/%d.csv", i + 1);
        sprintf(flowFilename, "flow_baseline/%d.csv", i + 1);

        float **pressureMat = (float **) malloc(sizeof(float *) * Duration);
        float **flowMat = (float **) malloc(sizeof(float *) * Duration);
        readFloatMatrixFromFile(pressureMat, Duration, Nnode, pressureFilename);
        readFloatMatrixFromFile(flowMat, Duration, Nlink, flowFilename);

        printf("read file %s done\n", pressureFilename);

        for (int x = 0; x < Duration; x++) {
            for (int y = 0; y < Nnode; y++) {
                u_pressure[x][y] += pressureMat[x][y];
                sigma_pressure[x][y] += pressureMat[x][y] * pressureMat[x][y];
            }
        }
        for (int x = 0; x < Duration; x++) {
            for (int y = 0; y < Nlink; y++) {
                u_flow[x][y] += flowMat[x][y];
                sigma_flow[x][y] += powf(flowMat[x][y], 2);
            }
        }
        free(pressureMat);
        free(flowMat);
    }

    for (int x = 0; x < Duration; x++) {
        for (int y = 0; y < Nnode; y++) {
            u_pressure[x][y] /= BASE_DEMAND_NUM;
            sigma_pressure[x][y] = sigma_pressure[x][y] / BASE_DEMAND_NUM - u_pressure[x][y] * u_pressure[x][y];
            sigma_pressure[x][y] = sqrtf(sigma_pressure[x][y]);
        }
    }
    for (int x = 0; x < Duration; x++) {
        for (int y = 0; y < Nlink; y++) {
            u_flow[x][y] /= BASE_DEMAND_NUM;
            sigma_flow[x][y] = sigma_flow[x][y] / BASE_DEMAND_NUM - u_flow[x][y] * u_flow[x][y];
            sigma_flow[x][y] = sqrtf(sigma_flow[x][y]);
        }
    }

    writeFloatMatrixToFile(*u_pressure, Duration, Nnode, "u_pressure.csv");
    writeFloatMatrixToFile(*sigma_pressure, Duration, Nnode, "sigma_pressure.csv");
    writeFloatMatrixToFile(*u_flow, Duration, Nlink, "u_flow.csv");
    writeFloatMatrixToFile(*sigma_flow, Duration, Nnode, "sigma_flow.csv");
}

void readFloatMatrixFromFile(float **mat, int rows, int cols, char *filename) {
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        return;
    }
    for (int i = 0; i < rows; i++) {
        mat[i] = (float *) malloc(sizeof(float) * cols);
        for (int j = 0; j < cols; j++) {
            fscanf(fp, j == 0 ? "\n%f" : ",%f", &mat[i][j]);
        }
    }
    fclose(fp);
}

void getEpanetInfo() {
    ENgetcount(EN_NODECOUNT, &Nall);
    ENgetcount(EN_TANKCOUNT, &Ntank);
    ENgetcount(EN_LINKCOUNT, &Nlink);
    long secondDuration;
    ENgettimeparam(EN_DURATION, &secondDuration);
    Nnode = Nall - Ntank;
    Nlink -= PUMP_NUM;
    Duration = secondDuration / 3600;
    printf("Nall = %d, Ntank = %d, Nnode = %d, Nlink = %d, Duration = %d\n", Nall, Ntank, Nnode, Nlink, Duration);
}