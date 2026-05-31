#pragma once

#include <iostream>
#include <string.h>
using namespace std;
#define MAX_USER 20

//用户结构体
typedef struct User
{
	char username[20];//用户名
	char password[20];//密码
	int role;//角色 0-管理员 1-用户
}User;

extern User users[MAX_USER];

extern int userCount;

void loadUser();

int login();

void userMenu();

void adminMenu();

