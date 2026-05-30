
#include <iostream>
using namespace std;
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
//用户结构体
typedef struct User
{
	char username[20];//用户名
	char password[20];//密码
	int role;//角色 0-管理员 1-用户
}User;
//全局变量
#define MAX_FLIGHT 100
#define MAX_USER 20
#define MAX_WAIT 100
Flight flight[MAX_FLIGHT];
User users[MAX_USER];
WaitingPassenger waitQueue[MAX_WAIT];
int front = 0;
int rear = 0;
int flightCount = 0;
int userCount = 0;
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
//读取用户
void loadUser()
{
    FILE* fp;
    fp = fopen("user.txt", "r");
    if (fp == NULL)
        return;
    while (fscanf(fp,"%s%s%d",users[userCount].username, users[userCount].password,&users[userCount].role) != EOF)
    {
        userCount++;
    }
    fclose(fp);
}
//登录
int login()
{
    char user[20];
    char pwd[20];
    int i;
    printf("用户名:");
    scanf("%s", user);
    printf("密码:");
    scanf("%s", pwd);

    for (i = 0;i < userCount;i++)
    {
        if (strcmp(user,users[i].username) == 0&&strcmp(pwd,users[i].password) == 0)
        {
            return users[i].role;
        }
    }
    return -1;
}
//读取航班
void loadFlight()
{
    FILE* fp;//打开航班文件
    fp = fopen("flight.txt", "r");
    if (fp == NULL)
    {
        printf("航班文件打开失败！\n");
        return;
    }
    while (fscanf(fp,"%s%s%s%s%s%s%f%d%d",
        flight[flightCount].flightNo,
        flight[flightCount].start,
        flight[flightCount].destination,
        flight[flightCount].date,
        flight[flightCount].startTime,
        flight[flightCount].arriveTime,
		&flight[flightCount].price,
        &flight[flightCount].totalSeat,
        &flight[flightCount].remainSeat)
        != EOF)
    {
        flight[flightCount].plist = NULL;
        flightCount++;
    }
    fclose(fp);//关闭文件
}
//显示全部航班函数
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
//生成订单编号函数
void generateOrderId(char orderId[])
{
    static int count = 1;

    sprintf(orderId,
        "OD%06d",
        count++);

}
//查询航班函数
int findFlight(char no[])
{
    int i;
    for (i = 0;i < flightCount;i++)
    {
        if (strcmp(flight[i].flightNo,no) == 0)
        {
            return i;
        }
    }
    return -1;
}
//目的地查询
void searchDestination()
{
    char key[20];
    int i;
    printf("请输入目的地:");
    scanf("%s", key);
    for (i = 0; i < flightCount; i++)
    {
        if (strstr(flight[i].destination,key) != NULL)
        {
            printf("\n航班号:%s\n",flight[i].flightNo);
            printf("航线:%s->%s\n",flight[i].start,flight[i].destination);
            printf("日期:%s\n",flight[i].date);
            printf("起飞:%s\n",flight[i].startTime);
            printf("到达:%s\n",flight[i].arriveTime);
            printf("票价:%.2f\n", flight[i].price);
            printf("余票:%d\n",flight[i].remainSeat);
        }
    }
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

    Passenger* p =(Passenger*)malloc(sizeof(Passenger));
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
        printf("\n余票不足！当前余票:%d\n",flight[pos].remainSeat);
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
    savePassengerToFile(p,flight[pos].flightNo);

    /* 计算总金额 */
    float totalMoney =flight[pos].price * p->ticketNum;

    printf("\n====================================\n");
    printf("               订票成功\n");
    printf("====================================\n");
    printf("订单编号：%s\n",p->orderId);
    printf("航班号：%s\n",flight[pos].flightNo);
    printf("航线：%s -> %s\n",flight[pos].start, flight[pos].destination);
    printf("日期：%s\n",flight[pos].date);
    printf("起飞时间：%s\n",flight[pos].startTime);
    printf("乘客姓名：%s\n",p->name);
    printf("联系电话：%s\n", p->phone);
    printf("身份证号：%s\n", p->id);
    printf("票价：%.2f 元\n",flight[pos].price);
    printf("购买数量：%d 张\n", p->ticketNum);
    printf("总金额：%.2f 元\n",totalMoney);
    printf("剩余票数：%d 张\n",flight[pos].remainSeat);
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
                printf("订单编号：%s\n",cur->orderId);
                printf("乘客姓名：%s\n",cur->name);
                printf("航班号：%s\n",flight[i].flightNo);
                printf("航线：%s -> %s\n",flight[i].start,flight[i].destination);
                printf("退票数量：%d 张\n",cur->ticketNum);
                printf("当前余票：%d 张\n",flight[i].remainSeat);
                printf("====================================\n");
                deletePassengerFromFile(orderId);
                free(cur);

                /* 候补自动补票 */
                if (!isEmpty())
                {
                    WaitingPassenger w =dequeue();
                    printf("\n候补乘客 %s 自动补票成功！\n",w.name);
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
        printf("航班号：%-12s 日期：%-12s\n", flight[i].flightNo,flight[i].date);
        printf("出发地：%-8s     目的地：%-8s\n",flight[i].start,flight[i].destination);
        printf("起飞时间：%-8s   到达时间：%-8s\n",flight[i].startTime,flight[i].arriveTime);
        printf("总座位：%-8d     剩余票数：%-8d\n",flight[i].totalSeat,flight[i].remainSeat);
        printf("----------------------------------------------------------\n");
        p = flight[i].plist;
        if (p == NULL)
        {
            printf("暂无订票客户！\n");
            continue;
        }
        printf("%-12s %-10s %-15s %-20s %-8s\n",
            "订单号",
            "姓名",
            "电话",
            "身份证",
            "票数");
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
//
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

    printf("%-10s %-10s %-10s %-15s %-20s %-6s\n",
        "订单号",
        "航班号",
        "姓名",
        "电话",
        "身份证",
        "票数");

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
//菜单函数
void menu()
{
    printf("\n");
    printf("=====航空票务管理系统=====\n");
    printf("1 显示航班\n");
    printf("2 查询目的地\n");
    printf("3 办理订票\n");
    printf("4 办理退票\n");
    printf("5 查看订票客户\n");
	printf("6 查看订单记录\n");
    printf("0 退出系统\n");
}
//主函数
int main()
{
    int role;
    int choice;
    loadUser();
    loadFlight();
    role = login();
    if (role == -1)
    {
        printf("登录失败\n");
        return 0;
    }
    while (1)
    {
        menu();
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            showFlight();
            break;

        case 2:
            searchDestination();
            break;

        case 3:
            bookTicket();
            break;

        case 4:
            refundTicket();
            break;

        case 5:
            showPassenger();
            break;

        case 6:
			showOrderFile();
			break;

        case 0:
            return 0;
        }
    }
}