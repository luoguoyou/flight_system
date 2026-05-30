
#include <iostream>
using namespace std;
//乘客信息结构体
typedef struct Passenger
{
    char name[20];
    int ticketNum;
    struct Passenger *next;
}Passenger;
//航班信息结构体
typedef struct Flight
{
    char flightNo[20];
    char start[20];
    char destination[20];

    char date[20];

    char startTime[20];
    char arriveTime[20];

    int totalSeat;
    int remainSeat;

    Passenger *plist;

}Flight;

//全局变量
#define MAX_FLIGHT 20
Flight flight[MAX_FLIGHT];
int flightCount=0;

//读取文件函数
void loadFlight()
{
    FILE *fp;

    fp=fopen("flight.txt","r");

    if(fp==NULL)
    {
        printf("文件打开失败\n");
        return;
    }

    while(fscanf(fp,
        "%s%s%s%s%s%s%d%d",
        flight[flightCount].flightNo,
        flight[flightCount].start,
        flight[flightCount].destination,
        flight[flightCount].date,
        flight[flightCount].startTime,
        flight[flightCount].arriveTime,
        &flight[flightCount].totalSeat,
        &flight[flightCount].remainSeat)!=EOF)
    {
        flight[flightCount].plist=NULL;
        flightCount++;
    }

    fclose(fp);
}
//显示全部航班函数
void showFlight()
{
    int i;

    printf("\n航班信息如下:\n");

    for(i=0;i<flightCount;i++)
    {
        printf("%s %s->%s 日期:%s 余票:%d\n",
            flight[i].flightNo,
            flight[i].start,
            flight[i].destination,
            flight[i].date,
            flight[i].remainSeat);
    }
}
/*按航班号查询*/
int findFlight(char no[])
{
    int i;

    for(i=0;i<flightCount;i++)
    {
        if(strcmp(flight[i].flightNo,no)==0)
            return i;
    }

    return -1;
}
//查询航班函数
void searchFlight()
{
    char no[20];

    printf("输入航班号:");
    scanf("%s",no);

    int pos=findFlight(no);

    if(pos==-1)
    {
        printf("未找到该航班\n");
        return;
    }

    printf("%s %s->%s\n",
        flight[pos].flightNo,
        flight[pos].start,
        flight[pos].destination);
}
//订票函数
void bookTicket()
{
    char no[20];
    char name[20];

    printf("输入航班号:");
    scanf("%s",no);

    int pos=findFlight(no);

    if(pos==-1)
    {
        printf("航班不存在\n");
        return;
    }

    if(flight[pos].remainSeat<=0)
    {
        printf("余票不足\n");
        return;
    }

    printf("输入姓名:");
    scanf("%s",name);

    Passenger *p;

    p=(Passenger*)malloc(sizeof(Passenger));

    strcpy(p->name,name);

    p->next=flight[pos].plist;

    flight[pos].plist=p;

    flight[pos].remainSeat--;

    printf("订票成功\n");
}
//退票函数
void refundTicket()
{
    char no[20];
    char name[20];

    printf("输入航班号:");
    scanf("%s",no);

    printf("输入姓名:");
    scanf("%s",name);

    int pos=findFlight(no);

    Passenger *pre=NULL;
    Passenger *cur=flight[pos].plist;

    while(cur)
    {
        if(strcmp(cur->name,name)==0)
        {
            if(pre==NULL)
                flight[pos].plist=cur->next;
            else
                pre->next=cur->next;

            free(cur);

            flight[pos].remainSeat++;

            printf("退票成功\n");
            return;
        }

        pre=cur;
        cur=cur->next;
    }

    printf("未找到该乘客\n");
}
//查看订票客户信息
void showPassenger()
{
    char no[20];

    printf("输入航班号:");
    scanf("%s",no);

    int pos=findFlight(no);

    Passenger *p=flight[pos].plist;

    while(p)
    {
        printf("%s\n",p->name);

        p=p->next;
    }
}
//菜单函数
void menu()
{
	cout << "==================" << endl;
	cout << "1.显示全部航班" << endl;
	cout << "2.查询航班" << endl;
	cout << "3.办理订票" << endl;
	cout << "4.办理退票" << endl;
	cout << "5.查看订票客户" << endl;
	cout << "0.退出系统" << endl;
	cout << "==================" << endl;
}
//主函数
int main()
{
    int choice;

    loadFlight();

    while(1)
    {
        menu();

        scanf("%d",&choice);

        switch(choice)
        {
        case 1:
            showFlight();
            break;

        case 2:
            searchFlight();
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

        case 0:
            exit(0);

        default:
            printf("输入错误\n");
        }
    }

    return 0;
}