/*
 * menu.h —— 菜单显示模块
 * 提供管理员和普通用户两种菜单界面。
 * 网络版客户端和单机版都使用同一套菜单定义。
 */
#pragma once

/* 显示普通用户菜单 */
void userMenu();

/* 显示管理员菜单 */
void adminMenu();

/* 单机版系统入口（不依赖网络，本地直接操作数据） */
void runSystem();
