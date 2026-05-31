#pragma once
#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;
#define MAX_FLIGHT 100

struct Passenger;

//航班信息结构体
typedef struct Flight
{
    char flightNo[20];      //航班号
    char start[20];         //出发地
    char destination[20];   //目的地
    char date[20];          //日期
    char startTime[20];     //起飞时间
    char arriveTime[20];    //到达时间
    float price;            //票价
    int totalSeat;          //总座位
    int remainSeat;         //余票
    Passenger* plist;       //乘客链表
}Flight;

extern Flight flight[MAX_FLIGHT];
extern int flightCount;

void loadFlight();
void saveFlight();

void showFlight();

int findFlight(char no[]);

void searchDestination();

void addFlight();
void deleteFlight();
void updateFlight();


