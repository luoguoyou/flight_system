/*
 * waitlist.h —— 候补队列管理模块
 * 使用循环数组实现的 FIFO 候补队列。
 * 当机票售罄时，乘客可以加入候补列表等待退票释放座位。
 * 一旦有退票，系统自动按候补顺序处理候补乘客。
 */
#pragma once

#include "data.h"

/* 将候补乘客加入队列尾部 */
int enqueueWaitPassenger(Passenger* p, char no[]);

/* 将候补队列保存到 wait.txt 文件 */
void saveWaitQueue();

/* 从 wait.txt 文件加载候补队列 */
void loadWaitQueue();

/* 查找指定航班的第一位候补乘客在队列中的索引 */
int findFirstWaitingIndex(char flightNo[]);

/* 移除队列中指定索引的候补乘客（自动向前移动后续元素） */
int removeWaitingAt(int index, WaitingPassenger* removedPassenger);

/* 单机版显示候补队列 */
void showWaitQueue();
