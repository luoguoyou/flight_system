
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
//航班保存函数
void saveFlight()
{
    FILE* fp;

    fp = fopen("flight.txt", "w");

    if (fp == NULL)
    {
        printf("保存失败！\n");
        return;
    }

    int i;

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
    char targetOrderId[20];

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

    while (fscanf(fp,
        "%s%s%s%s%s%d",
        orderId,
        flightNo,
        name,
        phone,
        id,
        &ticketNum) != EOF)
    {
        // 找到需要退票的订单
        if (strcmp(orderId, targetOrderId) == 0)
        {
            found = 1;

            int pos = findFlight(flightNo);

            if (pos != -1)
            {
                // 恢复余票
                flight[pos].remainSeat += ticketNum;

                printf("\n====================================\n");
                printf("                退票成功\n");
                printf("====================================\n");
                printf("订单编号：%s\n", orderId);
                printf("乘客姓名：%s\n", name);
                printf("航班号：%s\n", flightNo);
                printf("航线：%s -> %s\n",
                    flight[pos].start,
                    flight[pos].destination);
                printf("退票数量：%d 张\n", ticketNum);
                printf("当前余票：%d 张\n",
                    flight[pos].remainSeat);
                printf("====================================\n");
            }

            // 不写入temp，相当于删除订单
            continue;
        }

        // 保留其它订单
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

    // 用新文件替换旧文件
    remove("passenger.txt");
    rename("temp.txt", "passenger.txt");

    if (!found)
    {
        printf("未找到该订单号！\n");
        return;
    }

    // 保存更新后的航班余票
    saveFlight();

    // 候补处理
    if (!isEmpty())
    {
        WaitingPassenger w = dequeue();

        printf("\n候补乘客 %s 自动补票成功！\n",
            w.name);
    }
}
//显示所有航班及订票客户信息
void showPassenger()
{
    int i;

    printf("\n====================== 航班及订票客户信息 ======================\n");

    for (i = 0; i < flightCount; i++)
    {
        FILE* fp;

        char orderId[20];
        char flightNo[20];
        char name[20];
        char phone[20];
        char id[30];
        int ticketNum;

        int hasPassenger = 0;

        fp = fopen("passenger.txt", "r");

        if (fp == NULL)
        {
            printf("暂无订票记录！\n");
            return;
        }

        printf("\n");
        printf("==========================================================\n");
        printf("航班号：%-12s 日期：%-12s\n",
            flight[i].flightNo,
            flight[i].date);

        printf("出发地：%-8s     目的地：%-8s\n",
            flight[i].start,
            flight[i].destination);

        printf("起飞时间：%-8s   到达时间：%-8s\n",
            flight[i].startTime,
            flight[i].arriveTime);

        printf("总座位：%-8d     剩余票数：%-8d\n",
            flight[i].totalSeat,
            flight[i].remainSeat);

        printf("----------------------------------------------------------\n");

        while (fscanf(fp,
            "%s%s%s%s%s%d",
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
//修改航班函数
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

    printf("当前票价：%.2f\n",
        flight[pos].price);

    printf("输入新票价:");
    scanf("%f",
        &flight[pos].price);

    saveFlight();

    printf("修改成功！\n");
}
//航班增添函数
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
//航班删除函数
void deleteFlight()
{
    char no[20];

    printf("输入航班号:");

    scanf("%s", no);

    int pos = findFlight(no);

    if (pos == -1)
    {
        printf("航班不存在！\n");
        return;
    }

    int i;

    for (i = pos; i < flightCount - 1; i++)
    {
        flight[i] = flight[i + 1];
    }

    flightCount--;

    saveFlight();

    printf("删除成功！\n");
}
//用户界面
void userMenu()
{
    printf("\n");
    printf("========== 用户菜单 ==========\n");
    printf("1 显示航班\n");
    printf("2 查询航班\n");
    printf("3 办理订票\n");
    printf("4 办理退票\n");
    printf("5 查看订单记录\n");
    printf("0 退出系统\n");
}
//管理员界面
void adminMenu()
{
    printf("\n");
    printf("========== 管理员菜单 ==========\n");
    printf("1 显示航班\n");
    printf("2 查询航班\n");
    printf("3 新增航班\n");
    printf("4 删除航班\n");
    printf("5 修改航班\n");
    printf("6 查看订票客户\n");
    printf("7 查看订单记录\n");
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
    if (role == 0)
    {
        while (1)
        {
            adminMenu();
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
                addFlight();
                break;

            case 4:
                deleteFlight();
                break;

            case 5:
                updateFlight();
                break;

            case 6:
                showPassenger();
                break;

            case 7:
                showOrderFile();
                break;

            case 0:
                return 0;
            }
        }
    }
    else
    {
        while (1)
        {
            userMenu();

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
                showOrderFile();
                break;

            case 0:
                return 0;
            }
        }
    }
    
   
}