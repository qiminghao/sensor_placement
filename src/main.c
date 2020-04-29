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
#define TIME_STEP_MIN 0
#define TIME_STEP_MAX 119

void openepanet(char *f1, char *f2, char *f3);
int getRandomInt(int min, int max);
void printFloatArray(float arr[], int n, char str[]);
void randomBurst();

int main() {
    for (int i = 1; i <= 100; i++) {
        printf("the %dth times random burst: \n", i);
        randomBurst();
        printf("\n\n");
    }
    getchar();
    return 0;
}

void randomBurst() {
    openepanet("anytown.inp", "sample.rtp", "");//打开工具箱

    int Nall;
    int Ntank;
    int Nlink;
    int Nnode;
    ENgetcount(EN_NODECOUNT, &Nall);
    ENgetcount(EN_TANKCOUNT, &Ntank);
    ENgetcount(EN_LINKCOUNT, &Nlink);

    Nnode = Nall - Ntank;
    printf("Nall = %d, Ntank = %d, Nnode = %d, Nlink = %d\n", Nall, Ntank, Nnode, Nlink);

    int randomNode = getRandomInt(1, Nnode); // 随机的节点个数
    printf("randomNode = %d\n", randomNode);
    float value = (float) getRandomInt(EMITTER_MIN_VALUE, EMITTER_MAX_VALUE);
    ENsetnodevalue(randomNode, EN_EMITTER, value);
    printf("set the emitter of node %d as %.2f\n", randomNode, value);

    float pressure[Nnode + 1];
    float flow[Nlink + 1];
    memset(pressure, 0, sizeof(pressure));
    memset(flow, 0, sizeof(flow));

    int randomTimeStep = getRandomInt(TIME_STEP_MIN, TIME_STEP_MAX);
    printf("the selected random time step: %d\n", randomTimeStep);

    long t, tstep;
    ENopenH();
    ENinitH(0);
    do
    {
        ENrunH(&t);
        if (t == 3600 * randomTimeStep) {
            for (int index = 1; index <= Nnode; index++) {
                ENgetnodevalue(index, EN_PRESSURE, pressure + index);
            }
            for (int index = 1; index <= Nlink; index++) {
                ENgetlinkvalue(index, EN_FLOW, flow + index);
            }
            break;
        }
        /*
        检索时刻t的水力结果
        */
        ENnextH(&tstep);
    } while (tstep > 0);
    ENcloseH();
    ENclose();
    printFloatArray(pressure, Nnode, "pressure: ");
    printFloatArray(flow, Nlink, "flow: ");
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
    srand((unsigned int)time(NULL));
    int x = rand() % (max - min + 1) + min;
    Sleep(1000);
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

void printFloatArray(float arr[], int n, char str[]) {
    printf("%s", str);
    for (int i = 0; i < n; i++) {
        if (i == 0) {
            printf("[%f", arr[i]);
        } else {
            printf(", %f", arr[i]);
        }
    }
    printf("]\n");
}