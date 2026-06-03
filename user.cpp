#include "user.h"

#include "globals.h"
#include "utils.h"

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
