/*
 * ============================================================
 *   航班信息系统.c —— 程序入口
 *   根据命令行参数决定运行模式：
 *
 *   航班信息系统.exe server   → 启动服务端（监听 8888 端口）
 *   航班信息系统.exe local    → 启动单机版（不依赖网络）
 *   航班信息系统.exe          → 启动网络客户端（默认模式）
 *
 *   三种模式共享同一套数据结构（data.h）和业务逻辑，
 *   实现了单机版与网络版的无缝切换。
 * ============================================================
 */
#include "client.h"
#include "menu.h"
#include "server.h"

#include <string.h>

/*
 * main —— 程序主入口
 *
 * 命令行参数说明：
 * - 无参数：启动网络客户端，连接到本机 8888 端口的服务器
 * - server：启动 TCP 服务端，监听 8888 端口
 * - local： 启动单机版，不依赖网络直接操作本地文件
 */
int main(int argc, char* argv[])
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "server") == 0)
        {
            runServer();    /* 服务端模式 */
            return 0;
        }
        if (strcmp(argv[1], "local") == 0)
        {
            runSystem();    /* 单机本地模式 */
            return 0;
        }
    }
    runClient();            /* 默认：客户端模式 */
    return 0;
}
