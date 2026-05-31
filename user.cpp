#define _CRT_SECURE_NO_WARNINGS
#include "user.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;

User users[MAX_USER];
int userCount = 0;

//读取用户
void loadUser()
{
    FILE* fp;
    fp = fopen("user.txt", "r");
    if (fp == NULL)
        return;
    while (fscanf(fp, "%s%s%d", users[userCount].username, users[userCount].password, &users[userCount].role) != EOF)
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
        if (strcmp(user, users[i].username) == 0 && strcmp(pwd, users[i].password) == 0)
        {
            return users[i].role;
        }
    }
    return -1;
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