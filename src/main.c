#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc32-c"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include"epanet2.h"

#define INP_FILE_NAME "epanet/Austin net modified(24h).inp"
#define BURST_NUM 200
#define DAYS 360
#define NODE_NUM 126

void openepanet(char *f1, char *f2, char *f3);

void getWhiteNoise(char *filename, int len);

void getBurstSample(char *filename, int rows, int cols);

void writeFloatMatrixToFile(const float *arr, int rows, int cols, char *filename);

int burstSample[BURST_NUM][5];
float whiteNoise[DAYS];
float baseDemand[NODE_NUM + 1];
float pressure[288 * 360][4];
float initEmitter[NODE_NUM + 1];

int main() {

    memset(pressure, 0, sizeof(pressure));

    openepanet(INP_FILE_NAME, "sample.rtp", "");//打开工具箱

    getWhiteNoise("white_noise.csv", DAYS);

    getBurstSample("burst_sample.csv", BURST_NUM, 5);

    for (int i = 1; i <= NODE_NUM; i++) {
        ENgetnodevalue(i, EN_BASEDEMAND, &baseDemand[i]);
        ENgetnodevalue(i, EN_EMITTER, &initEmitter[i]);
    }

    char id[16];
    ENgetnodeid(52, id);
    printf("index:52 -> id:%s\n", id);
    ENgetnodeid(64, id);
    printf("index:64 -> id:%s\n", id);
    ENgetnodeid(114, id);
    printf("index:116 -> id:%s\n", id);
    ENgetnodeid(123, id);
    printf("index:123 -> id:%s\n", id);

    for (int i = 0; i < DAYS; i++) {
        int noise = whiteNoise[i];
        for (int j = 1; j <= NODE_NUM; j++) {
            ENsetnodevalue(i, EN_BASEDEMAND, baseDemand[i] + noise);
        }
        long t, tstep;
        ENopenH();
        ENinitH(0);
        do {
            ENrunH(&t);
            if (t % 300 == 0) {
                ENgetnodevalue(52, EN_PRESSURE, &pressure[i * 288 + t / 300][0]);
                ENgetnodevalue(64, EN_PRESSURE, &pressure[i * 288 + t / 300][1]);
                ENgetnodevalue(114, EN_PRESSURE, &pressure[i * 288 + t / 300][2]);
                ENgetnodevalue(123, EN_PRESSURE, &pressure[i * 288 + t / 300][3]);
            }
            ENnextH(&tstep);
        } while (tstep > 0);
        ENcloseH();
    }

    for (int i = 0; i < BURST_NUM; i++) {
        int day = burstSample[i][0];
        int start = burstSample[i][1];
        int interval = burstSample[i][2];
        int emitter = burstSample[i][3];
        int nodeIndex = burstSample[i][4];

        ENsetnodevalue(nodeIndex, EN_BASEDEMAND, baseDemand[i] + whiteNoise[day]);
        ENsetnodevalue(nodeIndex, EN_EMITTER, emitter);

        long t, tstep;
        ENopenH();
        ENinitH(0);
        do {
            ENrunH(&t);
            int step = t / 300;
            if (t % 300 == 0 && step >= start && step <= start + interval) {
                ENgetnodevalue(52, EN_PRESSURE, &pressure[day * 288 + step][0]);
                ENgetnodevalue(64, EN_PRESSURE, &pressure[day * 288 + step][1]);
                ENgetnodevalue(114, EN_PRESSURE, &pressure[day * 288 + step][2]);
                ENgetnodevalue(123, EN_PRESSURE, &pressure[day * 288 + step][3]);
            }
            ENnextH(&tstep);
        } while (tstep > 0);
        ENcloseH();

        ENsetnodevalue(nodeIndex, EN_EMITTER, initEmitter[nodeIndex]);
    }

    ENclose();

    writeFloatMatrixToFile(*pressure, 288 * 360, 4, "base.csv");

    return 0;
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

void getBurstSample(char *filename, int rows, int cols) {
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        return;
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fscanf(fp, j == 0 ? "\n%d" : ",%d", &burstSample[i][j]);
        }
    }
    fclose(fp);
}


void getWhiteNoise(char *filename, int len) {
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("open file %s failed!", filename);
        return;
    }
    for (int i = 0; i < len; i++) {
        fscanf(fp, "%f\n", &whiteNoise[i]);
    }
    fclose(fp);
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