#include "booking.h"

#include "flight_ops.h"
#include "globals.h"
#include "waitlist.h"

// 当某位候补乘客满足补票条件时，
// 把候补信息转成正式乘客订单并写入订单文件。
static void confirmWaitingPassenger(int flightPos, WaitingPassenger* w)
{
    Passenger* p = (Passenger*)malloc(sizeof(Passenger));

    if (p == NULL)
    {
        printf("系统内存不足，无法处理候补乘客！\n");
        return;
    }

    generateOrderId(p->orderId);
    strcpy(p->name, w->name);
    strcpy(p->phone, w->phone);
    strcpy(p->id, w->id);
    p->ticketNum = w->ticketNum;
    p->next = flight[flightPos].plist;
    flight[flightPos].plist = p;
    flight[flightPos].remainSeat -= p->ticketNum;
    savePassengerToFile(p, flight[flightPos].flightNo);

    printf("\n候补乘客 %s 已自动补票成功！\n", p->name);
    printf("订单编号：%s\n", p->orderId);
    printf("航班号：%s\n", flight[flightPos].flightNo);
    printf("补票数量：%d 张\n", p->ticketNum);
    printf("当前余票：%d 张\n", flight[flightPos].remainSeat);
}

// 某个航班有空余座位后，按候补顺序依次检查，
// 只要余票足够就为对应候补乘客自动补票。
static void processWaitingListForFlight(int flightPos)
{
    while (1)
    {
        int waitIndex = findFirstWaitingIndex(flight[flightPos].flightNo);

        if (waitIndex == -1)
        {
            break;
        }

        if (flight[flightPos].remainSeat < waitQueue[waitIndex].ticketNum)
        {
            break;
        }

        WaitingPassenger w;
        if (!removeWaitingAt(waitIndex, &w))
        {
            break;
        }

        confirmWaitingPassenger(flightPos, &w);
    }

    saveWaitQueue();
}

// 把一条订票记录追加保存到 passenger.txt。
void savePassengerToFile(Passenger* p, char flightNo[])
{
    FILE* fp = fopen("passenger.txt", "a");

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

// 按订单号从 passenger.txt 删除一条订票记录。
// 这里采用“读旧文件 + 写临时文件 + 替换原文件”的方式实现。
void deletePassengerFromFile(char orderId[])
{
    FILE* fp = fopen("passenger.txt", "r");
    FILE* temp = fopen("temp.txt", "w");
    char oId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;

    if (fp == NULL)
    {
        return;
    }

    while (fscanf(fp, "%s%s%s%s%s%d",
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
    rename("temp.txt", "passenger.txt");
}

// 本地版订票主流程：
// 先查找航班，再录入乘客信息；
// 余票足够时直接完成订票，余票不足时转入候补判断。
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
    if (p == NULL)
    {
        printf("内存分配失败！\n");
        return;
    }

    printf("请输入姓名:");
    scanf("%s", p->name);
    printf("请输入电话:");
    scanf("%s", p->phone);
    printf("请输入身份证号:");
    scanf("%s", p->id);
    printf("请输入订票数量:");
    scanf("%d", &p->ticketNum);

    if (p->ticketNum <= 0)
    {
        printf("订票数量必须大于0！\n");
        free(p);
        return;
    }

    // 如果余票不足，则询问用户是否加入候补队列。
    if (flight[pos].remainSeat < p->ticketNum)
    {
        int choice;

        printf("\n余票不足！当前余票:%d\n", flight[pos].remainSeat);
        printf("是否进入候补队列？(1-是 0-否):");
        scanf("%d", &choice);
        if (choice == 1)
        {
            if (enqueueWaitPassenger(p, no))
            {
                saveWaitQueue();
                printf("已加入候补队列！当前候补票数:%d\n", p->ticketNum);
            }
            else
            {
                printf("候补队列已满，加入失败！\n");
            }
        }

        free(p);
        return;
    }

    // 余票充足时，生成订单号并把乘客挂到该航班的订票链表上。
    generateOrderId(p->orderId);
    p->next = flight[pos].plist;
    flight[pos].plist = p;

    flight[pos].remainSeat -= p->ticketNum;
    savePassengerToFile(p, flight[pos].flightNo);
    saveFlight();

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

// 本地版退票主流程：
// 按订单号查找订单，退票后恢复航班余票，
// 最后再尝试处理该航班的候补队列。
void refundTicket()
{
    char targetOrderId[20];
    int waitingFlightPos = -1;

    printf("请输入订单号:");
    scanf("%s", targetOrderId);

    FILE* fp = fopen("passenger.txt", "r");
    if (fp == NULL)
    {
        printf("暂无订单记录！\n");
        return;
    }

    FILE* temp = fopen("temp.txt", "w");
    char orderId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;
    int found = 0;

    while (fscanf(fp, "%s%s%s%s%s%d",
        orderId,
        flightNo,
        name,
        phone,
        id,
        &ticketNum) != EOF)
    {
        if (strcmp(orderId, targetOrderId) == 0)
        {
            found = 1;

            int pos = findFlight(flightNo);
            if (pos != -1)
            {
                flight[pos].remainSeat += ticketNum;

                printf("\n====================================\n");
                printf("                退票成功\n");
                printf("====================================\n");
                printf("订单编号：%s\n", orderId);
                printf("乘客姓名：%s\n", name);
                printf("航班号：%s\n", flightNo);
                printf("航线：%s -> %s\n", flight[pos].start, flight[pos].destination);
                printf("退票数量：%d 张\n", ticketNum);
                printf("当前余票：%d 张\n", flight[pos].remainSeat);
                printf("====================================\n");

                waitingFlightPos = pos;
            }
            continue;
        }

        fprintf(temp,
            "%s %s %s %s %s %d\n",
            orderId,
            flightNo,
            name,
            phone,
            id,
            ticketNum);
    }

    fclose(fp);
    fclose(temp);

    remove("passenger.txt");
    rename("temp.txt", "passenger.txt");

    if (!found)
    {
        printf("未找到该订单号！\n");
        return;
    }

    saveFlight();
    if (waitingFlightPos != -1)
    {
        processWaitingListForFlight(waitingFlightPos);
        saveFlight();
    }
}
