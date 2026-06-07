/*
 * globals.h —— 全局变量声明
 * 供所有模块共享的全局数据，包括航班数组、
 * 用户数组、候补队列及其操作索引。
 */
#pragma once

#include "data.h"

extern Flight flight[MAX_FLIGHT];           /* 全局航班数组 */
extern User users[MAX_USER];                /* 全局用户数组 */
extern WaitingPassenger waitQueue[MAX_WAIT]; /* 候补队列（循环数组） */
extern int front;                           /* 候补队列队首指针 */
extern int rear;                            /* 候补队列队尾指针 */
extern int flightCount;                     /* 当前航班数量 */
extern int userCount;                       /* 当前用户数量 */
extern int nextOrderNumber;                 /* 下一个订单编号（自增） */
