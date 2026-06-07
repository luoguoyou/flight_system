/*
 * data.h —— 数据结构定义
 * 定义了系统核心数据结构：航班、乘客、候补乘客、用户。
 * 这些结构体在客户端和服务端共享使用。
 */
#pragma once

#include "common.h"

/* 乘客信息（用于订票记录） */
typedef struct Passenger {
    char orderId[20];        /* 订单编号（格式：OD000001） */
    char name[20];           /* 乘客姓名 */
    char phone[20];          /* 联系电话 */
    char id[20];             /* 身份证号 */
    int ticketNum;           /* 购票数量 */
    struct Passenger* next;  /* 链表指针 */
} Passenger;

/* 候补乘客信息（余票不足时排队等待） */
typedef struct WaitingPassenger {
    char flightNo[20];       /* 期望的航班号 */
    char name[20];           /* 姓名 */
    char phone[20];          /* 电话 */
    char id[20];             /* 身份证号 */
    int ticketNum;           /* 需要的票数 */
} WaitingPassenger;

/* 航班信息 */
typedef struct Flight {
    char flightNo[20];       /* 航班号 */
    char start[20];          /* 出发地 */
    char destination[20];    /* 目的地 */
    char date[20];           /* 日期 */
    char startTime[20];      /* 起飞时间 */
    char arriveTime[20];     /* 到达时间 */
    float price;             /* 票价 */
    int totalSeat;           /* 总座位数 */
    int remainSeat;          /* 剩余座位数 */
    struct Passenger* plist; /* 该航班乘客链表 */
} Flight;

/* 用户信息 */
typedef struct User {
    char username[20];       /* 用户名 */
    char password[20];       /* 密码 */
    int role;                /* 角色：0=管理员，1=普通用户 */
} User;

/* 数组容量常量 */
#define MAX_FLIGHT 100       /* 最大航班数 */
#define MAX_USER 20          /* 最大用户数 */
#define MAX_WAIT 100         /* 最大候补人数 */
