/*
 * user.h —— 用户管理模块
 * 提供用户数据的文件加载和登录验证功能。
 * 单机版使用这些函数完成用户认证。
 */
#pragma once

/* 从 user.txt 加载用户数据到全局数组 */
void loadUser();

/* 单机版登录功能（用户名密码验证） */
int login();
