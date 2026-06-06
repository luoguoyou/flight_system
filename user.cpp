#include "user.h"

#include "globals.h"
#include "utils.h"

// 从 user.txt 读取所有用户账号到全局用户数组。
void loadUser()
{
    FILE* fp = fopen("user.txt", "r");

    if (fp == NULL)
    {
        return;
    }

    while (fscanf(fp, "%s%s%d",
        users[userCount].username,
        users[userCount].password,
        &users[userCount].role) != EOF)
    {
        if (userCount == 0)
        {
            stripBom(users[userCount].username);
        }
        userCount++;
    }

    fclose(fp);
}

// 本地版登录流程：
// 输入用户名和密码后，在已加载的用户数组中逐个比对，
// 成功则返回角色，失败返回 -1。
int login()
{
    char user[20];
    char pwd[20];
    int i;

    printf("用户名:");
    scanf("%s", user);
    printf("密码:");
    scanf("%s", pwd);

    for (i = 0; i < userCount; i++)
    {
        if (strcmp(user, users[i].username) == 0 &&
            strcmp(pwd, users[i].password) == 0)
        {
            return users[i].role;
        }
    }

    return -1;
}
