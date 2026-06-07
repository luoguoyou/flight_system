#include "waitlist.h"

#include "globals.h"
#include "utils.h"

// 当前端已有空位被删除后，压缩候补数组，
// 避免 front 持续后移造成空间浪费。
static void compactWaitQueue()
{
    if (front == 0)
    {
        return;
    }

    int count = rear - front;
    int i;

    for (i = 0; i < count; i++)
    {
        waitQueue[i] = waitQueue[front + i];
    }

    front = 0;
    rear = count;
}

// 判断候补队列当前是否为空。
static int isEmpty()
{
    return front == rear;
}

// 将一位乘客加入候补队列。
// 如果数组尾部已满，会先尝试压缩前面已经空出的空间。
int enqueueWaitPassenger(Passenger* p, char no[])
{
    if (rear >= MAX_WAIT)
    {
        compactWaitQueue();
    }

    if (rear >= MAX_WAIT)
    {
        return 0;
    }

    strcpy(waitQueue[rear].flightNo, no);
    strcpy(waitQueue[rear].name, p->name);
    strcpy(waitQueue[rear].phone, p->phone);
    strcpy(waitQueue[rear].id, p->id);
    waitQueue[rear].ticketNum = p->ticketNum;
    rear++;
    return 1;
}

// 把当前候补队列保存到 wait.txt。
void saveWaitQueue()
{
    FILE* fp = fopen("wait.txt", "w");
    int i;

    if (fp == NULL)
    {
        printf("候补文件保存失败！\n");
        return;
    }

    for (i = front; i < rear; i++)
    {
        fprintf(fp,
            "%s %s %s %s %d\n",
            waitQueue[i].flightNo,
            waitQueue[i].name,
            waitQueue[i].phone,
            waitQueue[i].id,
            waitQueue[i].ticketNum);
    }

    fclose(fp);
}

// 从 wait.txt 加载候补队列到内存。
void loadWaitQueue()
{
    FILE* fp = fopen("wait.txt", "r");

    if (fp == NULL)
    {
        return;
    }

    front = 0;
    rear = 0;

    while (rear < MAX_WAIT && fscanf(fp,
        "%s%s%s%s%d",
        waitQueue[rear].flightNo,
        waitQueue[rear].name,
        waitQueue[rear].phone,
        waitQueue[rear].id,
        &waitQueue[rear].ticketNum) != EOF)
    {
        if (rear == 0)
        {
            stripBom(waitQueue[rear].flightNo);
        }
        rear++;
    }

    fclose(fp);
}

// 按航班号找到该航班在候补队列中的第一位乘客。
int findFirstWaitingIndex(char flightNo[])
{
    int i;

    for (i = front; i < rear; i++)
    {
        if (strcmp(waitQueue[i].flightNo, flightNo) == 0)
        {
            return i;
        }
    }

    return -1;
}

// 删除候补队列中指定位置的乘客。
// 如果调用方需要，也会把被删除的乘客信息带出去。
int removeWaitingAt(int index, WaitingPassenger* removedPassenger)
{
    int i;

    if (index < front || index >= rear)
    {
        return 0;
    }

    if (removedPassenger != NULL)
    {
        *removedPassenger = waitQueue[index];
    }

    for (i = index; i < rear - 1; i++)
    {
        waitQueue[i] = waitQueue[i + 1];
    }

    rear--;

    if (rear == front)
    {
        front = 0;
        rear = 0;
    }

    return 1;
}

// 以表格形式显示当前候补队列。
void showWaitQueue()
{
    int i;

    if (isEmpty())
    {
        printf("当前暂无候补乘客！\n");
        return;
    }

    printf("\n================================================================================\n");
    printf("%-6s %-10s %-10s %-15s %-20s %-6s\n",
        "序号",
        "航班号",
        "姓名",
        "电话",
        "身份证",
        "票数");
    printf("================================================================================\n");

    for (i = front; i < rear; i++)
    {
        printf("%-6d %-10s %-10s %-15s %-20s %-6d\n",
            i - front + 1,
            waitQueue[i].flightNo,
            waitQueue[i].name,
            waitQueue[i].phone,
            waitQueue[i].id,
            waitQueue[i].ticketNum);
    }

    printf("================================================================================\n");
}
