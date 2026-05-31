
#define _CRT_SECURE_NO_WARNINGS
#include "passenger.h"

#include <iostream>
#include <string.h>
#include "flight.h"
using namespace std;

WaitingPassenger waitQueue[MAX_WAIT];
int front = 0;
int rear = 0;
//入队
void enqueue(char name[], char no[])
{
    strcpy(waitQueue[rear].name, name);
    strcpy(waitQueue[rear].flightNo, no);
    rear++;
}
//出队
WaitingPassenger dequeue()
{
    return waitQueue[front++];
}
//判空
int isEmpty()
{
    return front == rear;
}
//生成订单编号函数
void generateOrderId(char orderId[])
{
    static int count = 1;

    sprintf(orderId,
        "OD%06d",
        count++);

}
//保存一个订单到文件
void savePassengerToFile(Passenger* p, char flightNo[])
{
    FILE* fp;

    fp = fopen("passenger.txt", "a");

    if (fp == NULL)
    {
        printf("乘客文件打开失败！\n");
        return;
    }

    fprintf(fp,
        "%s %s %s %s %s %d\n",
        p->orderId,
        flightNo,
        p->name,
        p->phone,
        p->id,
        p->ticketNum);

    fclose(fp);
}
//删除订单记录
void deletePassengerFromFile(char orderId[])
{
    FILE* fp;
    FILE* temp;

    char oId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;

    fp = fopen("passenger.txt", "r");

    temp = fopen("temp.txt", "w");

    if (fp == NULL)
        return;

    while (fscanf(fp,
        "%s%s%s%s%s%d",
        oId,
        flightNo,
        name,
        phone,
        id,
        &ticketNum) != EOF)
    {
        if (strcmp(oId, orderId) != 0)
        {
            fprintf(temp,
                "%s %s %s %s %s %d\n",
                oId,
                flightNo,
                name,
                phone,
                id,
                ticketNum);
        }
    }

    fclose(fp);
    fclose(temp);

    remove("passenger.txt");

    rename("temp.txt",
        "passenger.txt");
}
//订票函数
void bookTicket()
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

    Passenger* p = (Passenger*)malloc(sizeof(Passenger));
    printf("请输入姓名:");
    scanf("%s", p->name);
    printf("请输入电话:");
    scanf("%s", p->phone);
    printf("请输入身份证号:");
    scanf("%s", p->id);
    printf("请输入订票数量:");
    scanf("%d", &p->ticketNum);
    generateOrderId(p->orderId);
    /* 余票检查 */
    if (flight[pos].remainSeat < p->ticketNum)
    {
        printf("\n余票不足！当前余票:%d\n", flight[pos].remainSeat);
        printf("是否进入候补队列？(1-是 0-否):");
        int choice;
        scanf("%d", &choice);
        if (choice == 1)
        {
            enqueue(p->name, no);
            printf("已加入候补队列！\n");
        }
        free(p);
        return;
    }

    /* 插入链表 */
    p->next = flight[pos].plist;
    flight[pos].plist = p;

    /* 扣减余票 */
    flight[pos].remainSeat -= p->ticketNum;
    savePassengerToFile(p, flight[pos].flightNo);

    /* 计算总金额 */
    float totalMoney = flight[pos].price * p->ticketNum;

    printf("\n====================================\n");
    printf("               订票成功\n");
    printf("====================================\n");
    printf("订单编号：%s\n", p->orderId);
    printf("航班号：%s\n", flight[pos].flightNo);
    printf("航线：%s -> %s\n", flight[pos].start, flight[pos].destination);
    printf("日期：%s\n", flight[pos].date);
    printf("起飞时间：%s\n", flight[pos].startTime);
    printf("乘客姓名：%s\n", p->name);
    printf("联系电话：%s\n", p->phone);
    printf("身份证号：%s\n", p->id);
    printf("票价：%.2f 元\n", flight[pos].price);
    printf("购买数量：%d 张\n", p->ticketNum);
    printf("总金额：%.2f 元\n", totalMoney);
    printf("剩余票数：%d 张\n", flight[pos].remainSeat);
    printf("====================================\n");
}
//退票函数（按订单号退票）
void refundTicket()
{
    char orderId[20];
    printf("请输入订单号:");
    scanf("%s", orderId);
    int i;
    /* 遍历所有航班 */
    for (i = 0; i < flightCount; i++)
    {
        Passenger* pre = NULL;
        Passenger* cur = flight[i].plist;

        while (cur != NULL)
        {
            if (strcmp(cur->orderId, orderId) == 0)
            {
                /* 恢复余票 */
                flight[i].remainSeat += cur->ticketNum;

                /* 删除链表节点 */
                if (pre == NULL)
                {
                    flight[i].plist = cur->next;
                }
                else
                {
                    pre->next = cur->next;
                }
                printf("\n====================================\n");
                printf("                退票成功\n");
                printf("====================================\n");
                printf("订单编号：%s\n", cur->orderId);
                printf("乘客姓名：%s\n", cur->name);
                printf("航班号：%s\n", flight[i].flightNo);
                printf("航线：%s -> %s\n", flight[i].start, flight[i].destination);
                printf("退票数量：%d 张\n", cur->ticketNum);
                printf("当前余票：%d 张\n", flight[i].remainSeat);
                printf("====================================\n");
                deletePassengerFromFile(orderId);
                free(cur);

                /* 候补自动补票 */
                if (!isEmpty())
                {
                    WaitingPassenger w = dequeue();
                    printf("\n候补乘客 %s 自动补票成功！\n", w.name);
                }
                return;
            }
            pre = cur;
            cur = cur->next;
        }
    }
    printf("未找到该订单号！\n");
}
//显示所有航班及订票客户信息
void showPassenger()
{
    int i;
    Passenger* p;

    printf("\n====================== 航班及订票客户信息 ======================\n");

    for (i = 0; i < flightCount; i++)
    {
        printf("\n");
        printf("==========================================================\n");
        printf("航班号：%-12s 日期：%-12s\n", flight[i].flightNo, flight[i].date);
        printf("出发地：%-8s     目的地：%-8s\n", flight[i].start, flight[i].destination);
        printf("起飞时间：%-8s   到达时间：%-8s\n", flight[i].startTime, flight[i].arriveTime);
        printf("总座位：%-8d     剩余票数：%-8d\n", flight[i].totalSeat, flight[i].remainSeat);
        printf("----------------------------------------------------------\n");
        p = flight[i].plist;
        if (p == NULL)
        {
            printf("暂无订票客户！\n");
            continue;
        }
        printf("%-12s %-10s %-15s %-20s %-8s\n", "订单号", "姓名", "电话", "身份证", "票数");
        printf("==========================================================\n");
        printf("\n");
        while (p)
        {
            printf("%-12s %-10s %-15s %-20s %-8d\n",
                p->orderId,
                p->name,
                p->phone,
                p->id,
                p->ticketNum);
            p = p->next;
        }
    }
    printf("\n====================== 信息显示完毕 ======================\n");
}
//显示订单
void showOrderFile()
{
    FILE* fp;

    char orderId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;

    fp = fopen("passenger.txt", "r");

    if (fp == NULL)
    {
        printf("暂无订单记录！\n");
        return;
    }

    printf("\n================================================================================\n");

    printf("%-12s %-10s %-10s %-15s %-20s %-8s\n", "订单号", "航班号","姓名", "电话", "身份证", "票数");

    printf("================================================================================\n");

    while (fscanf(fp,
        "%s%s%s%s%s%d",
        orderId,
        flightNo,
        name,
        phone,
        id,
        &ticketNum) != EOF)
    {
        printf("%-10s %-10s %-10s %-15s %-20s %-6d\n",
            orderId,
            flightNo,
            name,
            phone,
            id,
            ticketNum);
    }

    printf("================================================================================\n");

    fclose(fp);
}