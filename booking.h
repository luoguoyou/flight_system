/*
 * booking.h —— 订票与退票操作模块
 * 提供乘客信息的持久化存储（写入 passenger.txt）、
 * 以及单机版的订票/退票功能。
 */
#pragma once

#include "data.h"

/* 将乘客订单信息保存到 passenger.txt 文件 */
void savePassengerToFile(Passenger* p, char flightNo[]);

/* 根据订单号从 passenger.txt 中删除对应的乘车记录 */
void deletePassengerFromFile(char orderId[]);

/* 单机版订票功能 */
void bookTicket();

/* 单机版退票功能 */
void refundTicket();
