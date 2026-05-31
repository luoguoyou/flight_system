#pragma once
  

#include <iostream>
#include <string>
#include <stdlib.h>
#include "flight.h"
using namespace std;
#define MAX_WAIT 100

//乘客链表
typedef struct Passenger
{
    char orderId[20];     //订单编号
    char name[20];      //名字
    char phone[20];     //电话
    char id[20];        //身份证
    int ticketNum;      //订票数量
    struct Passenger* next;//指向下一个乘客
}Passenger;

//候补队列
typedef struct WaitingPassenger
{
    char name[20];
    char flightNo[20];
}WaitingPassenger;

extern WaitingPassenger waitQueue[MAX_WAIT];

extern int front;
extern int rear;

void enqueue(char name[], char no[]);

WaitingPassenger dequeue();

int isEmpty();

void generateOrderId(char orderId[]);

void bookTicket();

void refundTicket();

void showPassenger();

void savePassengerToFile(Passenger* p,char flightNo[]);

void deletePassengerFromFile(char orderId[]);

void showOrderFile();
