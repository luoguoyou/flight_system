#include "menu.h"

#include "booking.h"
#include "flight_ops.h"
#include "user.h"
#include "utils.h"
#include "waitlist.h"

// 原本地系统的用户菜单，网络客户端现在按这套内容对齐。
void userMenu()
{
    printf("\n");
    printf("========== 用户菜单 ==========\n");
    printf("1 显示航班\n");
    printf("2 查询航班\n");
    printf("3 办理订票\n");
    printf("4 办理退票\n");
    printf("5 查看订单记录\n");
    printf("6 查看候补队列\n");
    printf("0 退出系统\n");
}

// 原本地系统的管理员菜单，网络客户端现在按这套内容对齐。
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
    printf("8 查看候补队列\n");
    printf("0 退出系统\n");
}

// 单机本地版本的系统入口。
// 这里会先加载文件数据、执行登录，再进入对应的用户或管理员菜单循环。
void runSystem()
{
    int role;
    int choice;

    initConsole();
    loadUser();
    loadFlight();
    loadWaitQueue();
    initOrderNumber();

    role = login();
    if (role == -1)
    {
        printf("登录失败\n");
        return;
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
            case 8:
                showWaitQueue();
                break;
            case 0:
                return;
            }
        }
    }

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
        case 6:
            showWaitQueue();
            break;
        case 0:
            return;
        }
    }
}
