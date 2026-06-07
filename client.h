/*
 * client.h —— 客户端入口
 * 声明客户端的主入口函数 runClient()，
 * 该函数在 main() 中被调用，启动整个客户端程序。
 */
#pragma once

/*
 * runClient —— 客户端主入口
 * 调用此函数进入客户端程序的主循环：
 * 1. 初始化网络连接
 * 2. 登录/注册
 * 3. 显示菜单并进行业务操作
 */
void runClient();
