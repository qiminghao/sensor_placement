#include <stdio.h>
#include <stdlib.h>
#include"epanet2.h"

void openepanet(char *f1, char *f2, char *f3);
float getRandomFloat(int start, int end);
int getRandomNodeNum(int start, int end);
int *getRandomNodeArray(int n);

int main() {
    printf("hello world\n");
    openepanet("anytown.inp", "sample.rtp", "");//打开工具箱

//    int a = getRandomNodeNum(1, 22); // 随机的节点个数
//    int *index = getRandomNodeArray(a);
//    for (int i = 0; i < a; i++) {
//        ENsetnodevalue(index[i], EN_EMITTER, getRandomFloat(1, 50));
//    }

    ENclose();

    getchar();
    return 0;
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
        return;
    }
    else
    {
        printf("open the file is successful!\n");

    }

}
