#define _CRT_SECURE_NO_WARNINGS
#include "flight.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "passenger.h"
using namespace std;
Flight flight[MAX_FLIGHT];
int flightCount = 0;

//读取航班
void loadFlight()
{
    FILE* fp;//打开航班文件
    fp = fopen("flight.txt", "r");
    if (fp == NULL)
    {
        printf("航班文件打开失败！\n");
        return;
    }
    while (fscanf(fp, "%s%s%s%s%s%s%f%d%d",
        flight[flightCount].flightNo,
        flight[flightCount].start,
        flight[flightCount].destination,
        flight[flightCount].date,
        flight[flightCount].startTime,
        flight[flightCount].arriveTime,
        &flight[flightCount].price,
        &flight[flightCount].totalSeat,
        &flight[flightCount].remainSeat)
        != EOF)
    {
        flight[flightCount].plist = NULL;
        flightCount++;
    }
    fclose(fp);//关闭文件
}
//航班信息保存函数
void saveFlight()
{
    FILE* fp;

    fp = fopen("flight.txt", "w");

    if (fp == NULL)
    {
        printf("保存失败！\n");
        return;
    }

    int i;

    for (i = 0; i < flightCount; i++)
    {
        fprintf(fp,
            "%s %s %s %s %s %s %.2f %d %d\n",
            flight[i].flightNo,
            flight[i].start,
            flight[i].destination,
            flight[i].date,
            flight[i].startTime,
            flight[i].arriveTime,
            flight[i].price,
            flight[i].totalSeat,
            flight[i].remainSeat);
    }

    fclose(fp);
}
//显示全部航班函数
void showFlight()
{
    int i;
    printf("\n");
    printf("====================================================================================================================\n");
    printf("%-10s %-8s %-8s %-12s %-10s %-10s %-10s %-10s %-10s \n", "航班号", "出发地", "目的地", "日期", "起飞", "到达", "票价", "总座位", "余票");
    printf("====================================================================================================================\n");

    for (i = 0; i < flightCount; i++)
    {
        printf(" %-10s %-8s %-8s %-12s %-10s %-10s %-10.2f %-10d %-10d\n",
            flight[i].flightNo,
            flight[i].start,
            flight[i].destination,
            flight[i].date,
            flight[i].startTime,
            flight[i].arriveTime,
            flight[i].price,
            flight[i].totalSeat,
            flight[i].remainSeat);
    }

    printf("====================================================================================================================\n");
}
//查询航班函数
int findFlight(char no[])
{
    int i;
    for (i = 0;i < flightCount;i++)
    {
        if (strcmp(flight[i].flightNo, no) == 0)
        {
            return i;
        }
    }
    return -1;
}
//目的地查询
void searchDestination()
{
    char key[20];
    int i;
    printf("请输入目的地:");
    scanf("%s", key);
    for (i = 0; i < flightCount; i++)
    {
        if (strstr(flight[i].destination, key) != NULL)
        {
            printf("\n航班号:%s\n", flight[i].flightNo);
            printf("航线:%s->%s\n", flight[i].start, flight[i].destination);
            printf("日期:%s\n", flight[i].date);
            printf("起飞:%s\n", flight[i].startTime);
            printf("到达:%s\n", flight[i].arriveTime);
            printf("票价:%.2f\n", flight[i].price);
            printf("余票:%d\n", flight[i].remainSeat);
        }
    }
}
//航班增添函数
void addFlight()
{
    Flight f;

    printf("航班号:");
    scanf("%s", f.flightNo);

    printf("出发地:");
    scanf("%s", f.start);

    printf("目的地:");
    scanf("%s", f.destination);

    printf("日期:");
    scanf("%s", f.date);

    printf("起飞时间:");
    scanf("%s", f.startTime);

    printf("到达时间:");
    scanf("%s", f.arriveTime);

    printf("票价:");
    scanf("%f", &f.price);

    printf("总座位:");
    scanf("%d", &f.totalSeat);

    f.remainSeat = f.totalSeat;

    f.plist = NULL;

    flight[flightCount++] = f;

    saveFlight();

    printf("新增成功！\n");
}
//航班删除函数
void deleteFlight()
{
    char no[20];

    printf("输入航班号:");

    scanf("%s", no);

    int pos = findFlight(no);

    if (pos == -1)
    {
        printf("航班不存在！\n");
        return;
    }

    int i;

    for (i = pos; i < flightCount - 1; i++)
    {
        flight[i] = flight[i + 1];
    }

    flightCount--;

    saveFlight();

    printf("删除成功！\n");
}
//修改航班函数
void updateFlight()
{
    char no[20];

    printf("请输入航班号:");
    scanf("%s", no);

    int pos = findFlight(no);

    if (pos == -1)
    {
        printf("航班不存在！\n");
        return;
    }

    printf("当前票价：%.2f\n",
        flight[pos].price);

    printf("输入新票价:");
    scanf("%f",
        &flight[pos].price);

    saveFlight();

    printf("修改成功！\n");
}