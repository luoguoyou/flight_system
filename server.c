/*
 * ============================================================
 *   server.c —— 服务端主程序
 *   功能：启动 TCP 服务器，监听客户端连接，
 *         为每个客户端创建独立线程处理请求。
 *   架构：多线程 + 临界区保护的并发处理模型。
 * ============================================================
 */
#include "server.h"
#include "service.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

/* 服务器响应缓冲区大小 */
#define MAX_RESPONSE 16384

/*
 * ClientArgs —— 客户端线程参数结构体
 * 传递给工作线程，包含 socket 句柄和客户端编号。
 */
typedef struct {
    SOCKET client;   /* 客户端 socket 句柄 */
    int clientNo;    /* 客户端编号（自增，用于日志标识） */
} ClientArgs;

/*
 * sendAll —— 循环发送，确保数据全部发出
 * TCP 的 send() 可能一次只发送部分数据，
 * 本函数循环发送直到全部发送完成。
 */
static BOOL sendAll(SOCKET client, const char* text, int len)
{
    const char* data = text;
    int left = len;
    while (left > 0) {
        int sent = send(client, data, left, 0);
        if (sent <= 0) return FALSE;
        data += sent;
        left -= sent;
    }
    return TRUE;
}

/*
 * readLine —— 从 socket 读取一行数据（以 \n 结尾）
 * 协议使用"一行一个包"的格式，按行读取可自然分隔请求。
 * 支持 Windows (\r\n) 和 Unix (\n) 换行格式。
 */
static BOOL readLine(SOCKET client, char* line, int maxLen)
{
    int pos = 0;
    char ch;
    while (pos < maxLen - 1) {
        int received = recv(client, &ch, 1, 0);
        if (received <= 0) return FALSE;
        if (ch == '\n') { line[pos] = '\0'; return TRUE; }
        if (ch != '\r') line[pos++] = ch;
    }
    line[pos] = '\0';
    return TRUE;
}

/*
 * handleClient —— 客户端处理线程
 * 每个客户端连接对应一个独立线程，循环处理该客户端的请求。
 * 流程：读请求行 → 调用业务处理 → 发送响应行
 */
static DWORD WINAPI handleClient(LPVOID param)
{
    ClientArgs* args = (ClientArgs*)param;
    SOCKET client = args->client;
    int clientNo = args->clientNo;
    free(args);

    printf("客户端 #%d 已连接\n", clientNo);

    char request[4096];       /* 接收请求的缓冲区 */
    char response[MAX_RESPONSE];  /* 存放响应结果 */

    while (readLine(client, request, sizeof(request))) {
        /* 调用业务处理函数 */
        handleClientRequest(request, response, sizeof(response));

        /* 末尾加换行符，完成"一行一个包"协议 */
        int respLen = (int)strlen(response);
        if (respLen > MAX_RESPONSE - 2) respLen = MAX_RESPONSE - 2;
        response[respLen] = '\n';
        response[respLen + 1] = '\0';

        if (!sendAll(client, response, respLen + 1)) break;
    }

    closesocket(client);
    printf("客户端 #%d 已断开\n", clientNo);
    return 0;
}

/*
 * runServer —— 服务端主函数
 *
 * 启动流程：
 * 1. 初始化控制台编码（UTF-8）
 * 2. 从文件加载航班、用户、候补队列数据
 * 3. 初始化 WinSock 库
 * 4. 创建 TCP 监听 socket（端口 8888）
 * 5. 主循环：accept() 等待客户端连接
 * 6. 每个客户端创建独立线程处理
 */
void runServer()
{
    initConsole();
    initServerData();  /* 加载文件数据到内存 */

    /* 初始化 Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WinSock 初始化失败\n");
        return;
    }

    /* 创建 TCP 监听 socket */
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("创建监听 socket 失败\n");
        WSACleanup();
        return;
    }

    /* 绑定本机任意地址 + 8888 端口 */
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8888);

    /* 允许端口重用 */
    int reuse = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("绑定 8888 端口失败，请确认端口未被占用\n");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("监听失败\n");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    printf("服务器已启动，监听端口 8888\n");
    printf("等待客户端连接...\n");

    volatile LONG clientCounter = 0;

    /* 主循环：不断接受新连接 */
    while (1) {
        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            printf("接收客户端连接失败\n");
            continue;
        }

        LONG clientNo = InterlockedIncrement(&clientCounter);
        ClientArgs* args = (ClientArgs*)malloc(sizeof(ClientArgs));
        if (!args) { closesocket(client); continue; }
        args->client = client;
        args->clientNo = (int)clientNo;

        HANDLE hThread = CreateThread(NULL, 0, handleClient, args, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        } else {
            printf("创建线程失败\n");
            closesocket(client);
            free(args);
        }
    }

    closesocket(listenSocket);
    WSACleanup();
}