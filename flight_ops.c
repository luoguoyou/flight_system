#include "flight_ops.h"

#include "globals.h"
#include "utils.h"

// 从 flight.txt 读取航班基础数据到全局航班数组。
void loadFlight()
{
    FILE* fp = fopen("flight.txt", "r");

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
        &flight[flightCount].remainSeat) != EOF)
    {
        if (flightCount == 0)
        {
            stripBom(flight[flightCount].flightNo);
        }
        flight[flightCount].plist = NULL;
        flightCount++;
    }

    fclose(fp);
}

// 以表格方式显示当前所有航班信息。
void showFlight()
{
    int i;

    printf("\n");
    printf("====================================================================================================================\n");
    printf("%-10s %-8s %-8s %-12s %-10s %-10s %-10s %-10s %-10s\n",
        "航班号",
        "出发地",
        "目的地",
        "日期",
        "起飞",
        "到达",
        "票价",
        "总座位",
        "余票");
    printf("====================================================================================================================\n");

    for (i = 0; i < flightCount; i++)
    {
        printf("%-10s %-8s %-8s %-12s %-10s %-10s %-10.2f %-10d %-10d\n",
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

// 生成递增的订单号，格式为 OD000001。
void generateOrderId(char orderId[])
{
    sprintf(orderId, "OD%06d", nextOrderNumber++);
}

// 把当前内存中的航班数组重新写回 flight.txt。
void saveFlight()
{
    FILE* fp = fopen("flight.txt", "w");
    int i;

    if (fp == NULL)
    {
        printf("保存失败！\n");
        return;
    }

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

// 按航班号在全局航班数组中查找对应位置。
int findFlight(char no[])
{
    int i;

    for (i = 0; i < flightCount; i++)
    {
        if (strcmp(flight[i].flightNo, no) == 0)
        {
            return i;
        }
    }

    return -1;
}

// 本地版按“目的地关键字”查询航班。
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

// 启动时扫描 passenger.txt，找到当前最大的订单号，
// 让新订单号可以在原有基础上继续递增。
void initOrderNumber()
{
    FILE* fp = fopen("passenger.txt", "r");
    char orderId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;
    int maxOrderNumber = 0;

    if (fp == NULL)
    {
        return;
    }

    while (fscanf(fp,
        "%s%s%s%s%s%d",
        orderId,
        flightNo,
        name,
        phone,
        id,
        &ticketNum) != EOF)
    {
        stripBom(orderId);
        if (strncmp(orderId, "OD", 2) == 0)
        {
            int currentNumber = atoi(orderId + 2);
            if (currentNumber > maxOrderNumber)
            {
                maxOrderNumber = currentNumber;
            }
        }
    }

    nextOrderNumber = maxOrderNumber + 1;
    fclose(fp);
}

// 管理员查看“按航班分组的订票客户信息”。
void showPassenger()
{
    int i;

    printf("\n====================== 航班及订票客户信息 ======================\n");

    for (i = 0; i < flightCount; i++)
    {
        FILE* fp = fopen("passenger.txt", "r");
        char orderId[20];
        char flightNo[20];
        char name[20];
        char phone[20];
        char id[30];
        int ticketNum;
        int hasPassenger = 0;

        if (fp == NULL)
        {
            printf("暂无订票记录！\n");
            return;
        }

        printf("\n");
        printf("==========================================================\n");
        printf("航班号：%-12s 日期：%-12s\n", flight[i].flightNo, flight[i].date);
        printf("出发地：%-8s     目的地：%-8s\n", flight[i].start, flight[i].destination);
        printf("起飞时间：%-8s   到达时间：%-8s\n", flight[i].startTime, flight[i].arriveTime);
        printf("总座位：%-8d     剩余票数：%-8d\n", flight[i].totalSeat, flight[i].remainSeat);
        printf("----------------------------------------------------------\n");

        while (fscanf(fp, "%s%s%s%s%s%d",
            orderId,
            flightNo,
            name,
            phone,
            id,
            &ticketNum) != EOF)
        {
            if (strcmp(flightNo, flight[i].flightNo) == 0)
            {
                if (!hasPassenger)
                {
                    printf("%-12s %-10s %-15s %-20s %-8s\n",
                        "订单号",
                        "姓名",
                        "电话",
                        "身份证",
                        "票数");
                    printf("==========================================================\n");
                    hasPassenger = 1;
                }

                printf("%-12s %-10s %-15s %-20s %-8d\n",
                    orderId,
                    name,
                    phone,
                    id,
                    ticketNum);
            }
        }

        if (!hasPassenger)
        {
            printf("暂无订票客户！\n");
        }

        fclose(fp);
    }

    printf("\n====================== 信息显示完毕 ======================\n");
}

// 直接查看 passenger.txt 中的全部订单记录。
void showOrderFile()
{
    FILE* fp = fopen("passenger.txt", "r");
    char orderId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;

    if (fp == NULL)
    {
        printf("暂无订单记录！\n");
        return;
    }

    printf("\n================================================================================\n");
    printf("%-10s %-10s %-10s %-15s %-20s %-6s\n",
        "订单号",
        "航班号",
        "姓名",
        "电话",
        "身份证",
        "票数");
    printf("================================================================================\n");

    while (fscanf(fp, "%s%s%s%s%s%d",
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

// 管理员按航班号修改票价。
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

    printf("当前票价：%.2f\n", flight[pos].price);
    printf("输入新票价:");
    scanf("%f", &flight[pos].price);

    saveFlight();
    printf("修改成功！\n");
}

// 管理员新增一条航班记录，并默认把余票设置为总座位数。
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

// 管理员按航班号删除航班。
// 删除方式是把后面的航班前移覆盖当前位置。
void deleteFlight()
{
    char no[20];
    int i;

    printf("输入航班号:");
    scanf("%s", no);

    int pos = findFlight(no);

    if (pos == -1)
    {
        printf("航班不存在！\n");
        return;
    }

    for (i = pos; i < flightCount - 1; i++)
    {
        flight[i] = flight[i + 1];
    }

    flightCount--;
    saveFlight();
    printf("删除成功！\n");
}
