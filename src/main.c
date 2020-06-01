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
#define TIME_INDEX_MAX 911
#define PUMP_NUM 3
#define BURST_NUM 100
#define INP_FILE_NAME "epanet/anytown_S1.inp"
#define MAX_NODE_SENSOR_NUM 4
#define MAX_LINK_SENSOR_NUM 4
#define DETECTION_TIME_CYCLE 48
#define EX_PRESSURE_FILENAME "u_pressure.csv"
#define SIGMA_PRESSURE_FILENAME "sigma_pressure.csv"
#define EX_FLOW_FILENAME "u_flow.csv"
#define SIGMA_FLOW_FILENAME "sigma_flow.csv"
#define BURST_TIME_INDEXES_FILENAME "burst_timeIndexes.csv"
#define NORMAL_TIME_INDEXES_FILENAME "normal_timeIndexes.csv"
#define PRESSURE_DETECTION_MATRIX_FILENAME "pressure_detection.csv"
#define FLOW_DETECTION_MATRIX_FILENAME "flow_detection.csv"

void openepanet(char *f1, char *f2, char *f3);

void getEpanetInfo();

int getRandomInt(int min, int max);

void printFloatArray(float arr[], int s, int e, char str[]);

void randomBurst();

void randomNormal();

void writeFloatMatrixToFile(const float *arr, int rows, int cols, char *filename);

void readFloatMatrixFromFile(float **mat, int rows, int cols, char *filename);

void readIntMatrixFromFile(int **mat, int rows, int cols, char *filename);

void writeIntMatrixToFile(const int *arr, int rows, int cols, char *filename);

void gauss(float *arr, float ex, float dx, int n);

void runBaseline(char *filename, char *flowFilename);

void generateBaselineFile();

void readIntVectorFromFile(int *vec, int n, char *filename);

void computeExAndSigma();

int *WECRules(float **ex, float **sigma, float **data, int rows, int cols, int timeIndex);

void computeDetectionMatrix();

void swap(int arr[], int i, int j);

void reverse(int arr[], int s, int e);

void nextPermutation(int arr[], int n);

int combinationNumber(int m, int n);

void generateRandomSensorLocation(int m, int n, char *filename);

int **matrixMultiply(int **mat1, int m1, int n1, int **mat2, int m2, int n2);

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

    printf("getEpanetInfo start\n");
    getEpanetInfo();

    printf("generateBaselineFile start\n");
    generateBaselineFile();

    printf("randomNormal start\n");
    randomNormal();

    printf("randomBurst strat\n");
    randomBurst();

    ENclose();

    printf("computeExAndSigma start\n");
    computeExAndSigma();

    printf("computeDetectionMatrix start\n");
    computeDetectionMatrix();

    for (int i = 1; i <= MAX_NODE_SENSOR_NUM; i++) {
        printf("%dth pressure generateRandomSensorLocation start\n", i);
        char filename[50];
        sprintf(filename, "sensorLocation_pressure/%d.csv", i);
        generateRandomSensorLocation(Nnode, i, filename);
    }

    for (int i = 1; i <= MAX_LINK_SENSOR_NUM; i++) {
        printf("%dth flow generateRandomSensorLocation start\n", i);
        char filename[50];
        sprintf(filename, "sensorLocation_flow/%d.csv", i);
        generateRandomSensorLocation(Nlink, i, filename);
    }

    printf("computeDetectionProbMatrix start\n");
    computeDetectionProbMatrix();

    printf("findOptimalVectors start\n");
    findOptimalVectors();

    return 0;
}

void randomNormal() {

    // 保存每次随机保管时间选取的随机时间节点
    int timeIndexes[BURST_NUM];
    memset(timeIndexes, 0, sizeof(timeIndexes));

    for (int i = 0; i < BURST_NUM; i++) {
        // 选取随机时间节点
        int randomTimeIndex = getRandomInt(TIME_INDEX_MIN, TIME_INDEX_MAX);
        // 将本次事件选取观测的随机时间节点保存至timeIndexes数组中，该数组在main函数中定义
        timeIndexes[i] = randomTimeIndex;
        // 保存所有随机爆管事件的压力数据，100 * 22的矩阵，从下标0开始
        float normalPressures[DETECTION_TIME_CYCLE][Nnode];
        // 保存所有随机爆管事件的流量数据，100 * 43的矩阵，从下标0开始
        float normalFlows[DETECTION_TIME_CYCLE][Nlink];
        memset(normalPressures, 0, sizeof(normalPressures));
        memset(normalFlows, 0, sizeof(normalFlows));

        long t, tstep;
        ENopenH();
        ENinitH(0);
        do {
            ENrunH(&t);
            if (t % 3600 == 0) {
                int h = t / 3600;
                if (h >= randomTimeIndex + DETECTION_TIME_CYCLE) {
                    break;
                }
                // 当当前时刻等于选取的时间节点时，观测每个节点的压力和每个管段的流量
                if (h >= randomTimeIndex && h < randomTimeIndex + DETECTION_TIME_CYCLE) {
                    // 观测每个节点的压力，将结果保存到pressure数组，从下标1开始
                    for (int index = 0; index < Nnode; index++) {
                        ENgetnodevalue(index + 1, EN_PRESSURE, &normalPressures[h - randomTimeIndex][index]);
                    }
                    // 观测每个管段的流量，flow，从下标1开始
                    for (int index = 0; index < Nlink; index++) {
                        ENgetlinkvalue(index + 1, EN_FLOW, &normalFlows[h - randomTimeIndex][index]);
                    }
                }
            }
            ENnextH(&tstep);
        } while (tstep > 0);
        ENcloseH();

        char pressureFilename[50];
        char flowFilename[50];
        sprintf(pressureFilename, "pressure_normal/%d.csv", i + 1);
        sprintf(flowFilename, "flow_normal/%d.csv", i + 1);
        writeFloatMatrixToFile(*normalPressures, DETECTION_TIME_CYCLE, Nnode, pressureFilename);
        writeFloatMatrixToFile(*normalFlows, DETECTION_TIME_CYCLE, Nlink, flowFilename);
    }
    writeIntMatrixToFile(timeIndexes, 1, BURST_NUM, NORMAL_TIME_INDEXES_FILENAME);
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
    // 保存每次随机保管时间选取的随机时间节点
    int timeIndexes[BURST_NUM];
    memset(timeIndexes, 0, sizeof(timeIndexes));

    for (int i = 0; i < BURST_NUM; i++) {
        // 选取随机节点
        int randomNode = getRandomInt(1, Nnode);
        // 选取随机的扩散器系数
        float value = (float) getRandomInt(EMITTER_MIN_VALUE, EMITTER_MAX_VALUE);
        // 设置节点的扩散器系数
        ENsetnodevalue(randomNode, EN_EMITTER, value);
        // 选取随机时间节点
        int randomTimeIndex = getRandomInt(TIME_INDEX_MIN, TIME_INDEX_MAX);
        // 将本次事件选取观测的随机时间节点保存至timeIndexes数组中，该数组在main函数中定义
        timeIndexes[i] = randomTimeIndex;
        // 保存所有随机爆管事件的压力数据，100 * 22的矩阵，从下标0开始
        float burstPressures[DETECTION_TIME_CYCLE][Nnode];
        // 保存所有随机爆管事件的流量数据，100 * 43的矩阵，从下标0开始
        float burstFlows[DETECTION_TIME_CYCLE][Nlink];
        memset(burstPressures, 0, sizeof(burstPressures));
        memset(burstFlows, 0, sizeof(burstFlows));

        long t, tstep;
        ENopenH();
        ENinitH(0);
        do {
            ENrunH(&t);
            if (t % 3600 == 0) {
                int h = t / 3600;
                if (h >= randomTimeIndex + DETECTION_TIME_CYCLE) {
                    break;
                }
                // 当当前时刻等于选取的时间节点时，观测每个节点的压力和每个管段的流量
                if (h >= randomTimeIndex && h < randomTimeIndex + DETECTION_TIME_CYCLE) {
                    // 观测每个节点的压力，将结果保存到pressure数组，从下标1开始
                    for (int index = 0; index < Nnode; index++) {
                        ENgetnodevalue(index + 1, EN_PRESSURE, &burstPressures[h - randomTimeIndex][index]);
                    }
                    // 观测每个管段的流量，flow，从下标1开始
                    for (int index = 0; index < Nlink; index++) {
                        ENgetlinkvalue(index + 1, EN_FLOW, &burstFlows[h - randomTimeIndex][index]);
                    }
                }
            }
            ENnextH(&tstep);
        } while (tstep > 0);
        ENcloseH();

        char pressureFilename[50];
        char flowFilename[50];
        sprintf(pressureFilename, "pressure_burst/%d.csv", i + 1);
        sprintf(flowFilename, "flow_burst/%d.csv", i + 1);
        writeFloatMatrixToFile(*burstPressures, DETECTION_TIME_CYCLE, Nnode, pressureFilename);
        writeFloatMatrixToFile(*burstFlows, DETECTION_TIME_CYCLE, Nlink, flowFilename);

        // 在每次随机爆管事件发生后，还原所有节点的扩散器系数
        for (int j = 1; j <= Nnode; j++) {
            ENsetnodevalue(j, EN_EMITTER, initEmitters[j]);
        }
    }
    writeIntMatrixToFile(timeIndexes, 1, BURST_NUM, BURST_TIME_INDEXES_FILENAME);
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

    float baseDemand[BASE_DEMAND_NUM][Nnode];
    memset(baseDemand, 0, sizeof(baseDemand));

    for (int i = 0; i < Nnode; i++) {
        float *temp;
        temp = (float *) malloc(BASE_DEMAND_NUM * sizeof(float));
        gauss(temp, initBaseDemand[i], 1, BASE_DEMAND_NUM);
        for (int j = 0; j < BASE_DEMAND_NUM; j++) {
            baseDemand[j][i] = temp[j];
        }
        free(temp);
    }

    for (int i = 0; i < BASE_DEMAND_NUM; i++) {
        char pressureFilename[50];
        char flowFilename[50];
        sprintf(pressureFilename, "pressure_baseline/%d.csv", i + 1);
        sprintf(flowFilename, "flow_baseline/%d.csv", i + 1);

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

    writeFloatMatrixToFile(*u_pressure, Duration, Nnode, EX_PRESSURE_FILENAME);
    writeFloatMatrixToFile(*sigma_pressure, Duration, Nnode, SIGMA_PRESSURE_FILENAME);
    writeFloatMatrixToFile(*u_flow, Duration, Nlink, EX_FLOW_FILENAME);
    writeFloatMatrixToFile(*sigma_flow, Duration, Nnode, SIGMA_FLOW_FILENAME);
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

void readIntMatrixFromFile(int **mat, int rows, int cols, char *filename) {
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        return;
    }
    for (int i = 0; i < rows; i++) {
        mat[i] = (int *) malloc(sizeof(int) * cols);
        for (int j = 0; j < cols; j++) {
            fscanf(fp, j == 0 ? "\n%d" : ",%d", &mat[i][j]);
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

void readIntVectorFromFile(int *vec, int n, char *filename) {
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        return;
    }
    for (int i = 0; i < n; i++) {
        fscanf(fp, i == 0 ? "%d" : ",%d", vec + i);
    }
    fclose(fp);
}

int *WECRules(float **ex, float **sigma, float **data, int rows, int cols, int timeIndex) {
    int *res = (int *) malloc(cols * sizeof(int));
    for (int j = 0; j < cols; j++) {
        int beyondFourSigma = 0;
        int beyondThreeSigma = 0;
        int beyondTwoSigma = 0;
        int beyondOneSigma = 0;
        for (int i = 0; i < rows; i++) {
            if (data[i][j] > ex[i + timeIndex][j] + 4 * sigma[i + timeIndex][j] ||
                data[i][j] < ex[i + timeIndex][j] - 4 * sigma[i + timeIndex][j]) {
                beyondFourSigma++;
            }
            if (data[i][j] > ex[i + timeIndex][j] + 3 * sigma[i + timeIndex][j] ||
                data[i][j] < ex[i + timeIndex][j] - 3 * sigma[i + timeIndex][j]) {
                beyondThreeSigma++;
            }
            if (data[i][j] > ex[i + timeIndex][j] + 2 * sigma[i + timeIndex][j] ||
                data[i][j] < ex[i + timeIndex][j] - 2 * sigma[i + timeIndex][j]) {
                beyondTwoSigma++;
            }
            if (data[i][j] > ex[i + timeIndex][j] + sigma[i + timeIndex][j] ||
                data[i][j] < ex[i + timeIndex][j] - sigma[i + timeIndex][j]) {
                beyondOneSigma++;
            }
        }
        if (beyondFourSigma > 0 ||
            (float) beyondThreeSigma / rows > 2.0 / 3 ||
            (float) beyondTwoSigma / rows > 4.0 / 5 ||
            (float) beyondOneSigma / rows > 7.0 / 8) {
            res[j] = 1;
        } else {
            res[j] = 0;
        }
    }
    return res;
}

void computeDetectionMatrix() {
    float **u_pressure = (float **) malloc(Duration * sizeof(float *));
    readFloatMatrixFromFile(u_pressure, Duration, Nnode, EX_PRESSURE_FILENAME);

    float **sigma_pressure = (float **) malloc(Duration * sizeof(float *));
    readFloatMatrixFromFile(sigma_pressure, Duration, Nnode, SIGMA_PRESSURE_FILENAME);

    float **u_flow = (float **) malloc(Duration * sizeof(float *));
    readFloatMatrixFromFile(u_flow, Duration, Nlink, EX_FLOW_FILENAME);

    float **sigma_flow = (float **) malloc(Duration * sizeof(float *));
    readFloatMatrixFromFile(sigma_flow, Duration, Nlink, SIGMA_FLOW_FILENAME);

    int *burstTimeIndexes = (int *) malloc(BURST_NUM * sizeof(int));
    readIntVectorFromFile(burstTimeIndexes, BURST_NUM, BURST_TIME_INDEXES_FILENAME);

    int pressureDetectionMatrix[BURST_NUM][Nnode];
    int flowDetectionMatrix[BURST_NUM][Nlink];
    for (int i = 0; i < BURST_NUM; i++) {
        char burstPressureFilename[50];
        sprintf(burstPressureFilename, "pressure_burst/%d.csv", i + 1);
        float **burstPressure = (float **) malloc(DETECTION_TIME_CYCLE * sizeof(float *));
        readFloatMatrixFromFile(burstPressure, DETECTION_TIME_CYCLE, Nnode, burstPressureFilename);
        int *pressureRes = WECRules(u_pressure, sigma_pressure, burstPressure, DETECTION_TIME_CYCLE, Nnode,
                                    burstTimeIndexes[i]);
        for (int j = 0; j < Nnode; j++) {
            pressureDetectionMatrix[i][j] = pressureRes[j];
        }

        char burstFlowFilename[50];
        sprintf(burstFlowFilename, "flow_burst/%d.csv", i + 1);
        float **burstFlow = (float **) malloc(DETECTION_TIME_CYCLE * sizeof(float *));
        readFloatMatrixFromFile(burstFlow, DETECTION_TIME_CYCLE, Nlink, burstFlowFilename);
        int *flowRes = WECRules(u_flow, sigma_flow, burstFlow, DETECTION_TIME_CYCLE, Nlink, burstTimeIndexes[i]);
        for (int j = 0; j < Nlink; j++) {
            flowDetectionMatrix[i][j] = flowRes[j];
        }
    }
    writeIntMatrixToFile(*pressureDetectionMatrix, BURST_NUM, Nnode, PRESSURE_DETECTION_MATRIX_FILENAME);
    writeIntMatrixToFile(*flowDetectionMatrix, BURST_NUM, Nlink, FLOW_DETECTION_MATRIX_FILENAME);
    free(u_pressure);
    free(sigma_pressure);
    free(u_flow);
    free(sigma_flow);
}

void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 数组区间[s, e]反转
// [0,1,2,3,4,5,6,7,8,9], 3, 8
// [0,1,2,8,7,6,5,4,3,9]
void reverse(int arr[], int s, int e) {
    for (int i = 0; i < (e - s + 1) / 2; i++) {
        swap(arr, s + i, e - i);
    }
}

// 下一个排列
// [0,0,0,0,1,1], 6
// [0,0,0,1,0,1]
void nextPermutation(int arr[], int n) {
    int i = n - 2;
    while (i >= 0 && arr[i] >= arr[i + 1]) {
        i--;
    }
    if (i >= 0) {
        int j = i + 1;
        while (j < n && arr[j] > arr[i]) {
            j++;
        }
        j--;
        swap(arr, i, j);
        reverse(arr, i + 1, n - 1);
    }
}

// 注意数值溢出问题，大数量级组合数用对数计算
int combinationNumber(int m, int n) {
    if (m < n) {
        return 0;
    }
    if (n > m / 2) {
        n = m - n;
    }
    long long res = 1;
    for (int i = m; i > m - n; i--) {
        res *= i;
    }
    for (int i = n; i > 1; i--) {
        res /= i;
    }
    return (int) res;
}

void generateRandomSensorLocation(int m, int n, char *filename) {
    int count = combinationNumber(m, n);
    int sensorLocation[m][count];
    int arr[m];
    memset(arr, 0, sizeof(arr));
    for (int i = 1; i <= n; i++) {
        arr[m - i] = 1;
    }
    for (int j = 0; j < count; j++) {
        for (int i = 0; i < m; i++) {
            sensorLocation[i][j] = arr[i];
        }
        nextPermutation(arr, m);
    }
    writeIntMatrixToFile(*sensorLocation, m, count, filename);
}

int **matrixMultiply(int **mat1, int m1, int n1, int **mat2, int m2, int n2) {
    if (n1 != m2) {
        return NULL;
    }
    if (mat1 == NULL || *mat1 == NULL || mat2 == NULL || *mat2 == NULL) {
        return NULL;
    }
    int **result = (int **) malloc(m1 * sizeof(int *));
    for (int i = 0; i < m1; i++) {
        result[i] = (int *) malloc(n2 * sizeof(int));
    }
    for (int i = 0; i < m1; i++) {
        for (int j = 0; j < n2; j++) {
            int temp = 0;
            for (int k = 0; k < n1; k++) {
                temp += mat1[i][k] * mat2[k][j];
            }
            result[i][j] = temp;
        }
    }
    return result;
}

void computeDetectionProbMatrix() {
    int **pressureDetectionMat = (int **) malloc(BURST_NUM * sizeof(int *));
    readIntMatrixFromFile(pressureDetectionMat, BURST_NUM, Nnode, PRESSURE_DETECTION_MATRIX_FILENAME);
    for (int i = 1; i <= MAX_NODE_SENSOR_NUM; i++) {
        int count = combinationNumber(Nnode, i);
        char filename[50];
        sprintf(filename, "sensorLocation_pressure/%d.csv", i);
        int **sensorLocation = (int **) malloc(Nnode * sizeof(int *));
        readIntMatrixFromFile(sensorLocation, Nnode, count, filename);
        int **product = matrixMultiply(pressureDetectionMat, BURST_NUM, Nnode, sensorLocation, Nnode, count);
        int res[BURST_NUM][count];
        for (int x = 0; x < BURST_NUM; x++) {
            for (int y = 0; y < count; y++) {
                res[x][y] = product[x][y] == 0 ? 0 : 1;
            }
        }
        sprintf(filename, "detectionResultMatrix_pressure/%d.csv", i);
        writeIntMatrixToFile(*res, BURST_NUM, count, filename);
        free(sensorLocation);
        free(product);
    }

    int **flowDetectionMat = (int **) malloc(BURST_NUM * sizeof(int *));
    readIntMatrixFromFile(flowDetectionMat, BURST_NUM, Nlink, FLOW_DETECTION_MATRIX_FILENAME);
    for (int i = 1; i <= MAX_LINK_SENSOR_NUM; i++) {
        int count = combinationNumber(Nlink, i);
        char filename[50];
        sprintf(filename, "sensorLocation_flow/%d.csv", i);
        int **sensorLocation = (int **) malloc(Nlink * sizeof(int *));
        readIntMatrixFromFile(sensorLocation, Nnode, count, filename);
        int **product = matrixMultiply(pressureDetectionMat, BURST_NUM, Nnode, sensorLocation, Nnode, count);
        int res[BURST_NUM][count];
        for (int x = 0; x < BURST_NUM; x++) {
            for (int y = 0; y < count; y++) {
                res[x][y] = product[x][y] == 0 ? 0 : 1;
            }
        }
        sprintf(filename, "detectionResultMatrix_flow/%d.csv", i);
        writeIntMatrixToFile(*res, BURST_NUM, count, filename);
        free(sensorLocation);
        free(product);
    }
    free(pressureDetectionMat);
    free(flowDetectionMat);
}

void findOptimalVectors() {
    char filename[50];

    for (int i = 1; i <= MAX_NODE_SENSOR_NUM; i++) {
        int count = combinationNumber(Nnode, i);
        int **detectionRes = (int **) malloc(BURST_NUM * sizeof(int *));
        sprintf(filename, "detectionResultMatrix_pressure/%d.csv", i);
        readIntMatrixFromFile(detectionRes, BURST_NUM, count, filename);

        int stat[count];
        int maxCount = -1;
        memset(stat, 0, sizeof(stat));
        for (int y = 0; y < count; y++) {
            int sum = 0;
            for (int x = 0; x < BURST_NUM; x++) {
                sum += detectionRes[x][y];
            }
            maxCount = sum > maxCount ? sum : maxCount;
            stat[y] = sum;
        }
        int maxColumns[count + 1];
        memset(maxColumns, 0, sizeof(maxColumns));
        int index = 0;
        for (int j = 0; j < count; j++) {
            if (maxCount == stat[j]) {
                maxColumns[index++] = j;
            }
        }

        int chosenSensorLocation[Nnode][index];
        sprintf(filename, "sensorLocation_pressure/%d.csv", i);
        int **sensorLocation = (int **) malloc(Nnode * sizeof(int *));
        readIntMatrixFromFile(sensorLocation, Nnode, count, filename);
        for (int j = 0; j < index; j++) {
            for (int x = 0; x < Nnode; x++) {
                chosenSensorLocation[x][j] = sensorLocation[x][maxColumns[j]];
            }
        }
        sprintf(filename, "chosenSensorLocation_pressure/%d.csv", i);
        writeIntMatrixToFile(*chosenSensorLocation, Nnode, index, filename);
        sprintf(filename, "chosenSensorLocation_pressure/%d_columnNumber.csv", i);
        writeIntMatrixToFile(&index, 1, 1, filename);
        free(detectionRes);
        free(sensorLocation);
    }

    for (int i = 1; i <= MAX_LINK_SENSOR_NUM; i++) {
        int count = combinationNumber(Nlink, i);
        int **detectionRes = (int **) malloc(BURST_NUM * sizeof(int *));
        sprintf(filename, "detectionResultMatrix_flow/%d.csv", i);
        readIntMatrixFromFile(detectionRes, BURST_NUM, count, filename);

        int stat[count];
        int maxCount = -1;
        memset(stat, 0, sizeof(stat));
        for (int y = 0; y < count; y++) {
            int sum = 0;
            for (int x = 0; x < BURST_NUM; x++) {
                sum += detectionRes[x][y];
            }
            maxCount = sum > maxCount ? sum : maxCount;
            stat[y] = sum;
        }
        int maxColumns[count + 1];
        memset(maxColumns, 0, sizeof(maxColumns));
        int index = 0;
        for (int j = 0; j < count; j++) {
            if (maxCount == stat[j]) {
                maxColumns[index++] = j;
            }
        }

        int chosenSensorLocation[Nlink][index];

        sprintf(filename, "sensorLocation_flow/%d.csv", i);
        int **sensorLocation = (int **) malloc(Nlink * sizeof(int *));
        readIntMatrixFromFile(sensorLocation, Nlink, count, filename);
        for (int j = 0; j < index; j++) {
            for (int x = 0; x < Nnode; x++) {
                chosenSensorLocation[x][j] = sensorLocation[x][maxColumns[j]];
            }
        }
        sprintf(filename, "chosenSensorLocation_flow/%d.csv", i);
        writeIntMatrixToFile(*chosenSensorLocation, Nnode, index, filename);
        sprintf(filename, "chosenSensorLocation_flow/%d_columnNumber.csv", i);
        writeIntMatrixToFile(&index, 1, 1, filename);
        free(detectionRes);
        free(sensorLocation);
    }
}