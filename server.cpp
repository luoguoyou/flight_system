#include "server.h"

#include "service.h"
#include "utils.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

// 确保一整段响应文本被完整发送给客户端。
static bool sendAll(SOCKET client, const std::string& text)
{
    const char* data = text.c_str();
    int left = (int)text.size();

    while (left > 0)
    {
        int sent = send(client, data, left, 0);
        if (sent <= 0)
        {
            return false;
        }
        data += sent;
        left -= sent;
    }

    return true;
}

// 按“单行文本协议”从 socket 中读取一条完整请求。
// 以换行符作为一条请求结束的标记。
static bool readLine(SOCKET client, std::string& line)
{
    line.clear();
    char ch = '\0';

    while (true)
    {
        int received = recv(client, &ch, 1, 0);
        if (received <= 0)
        {
            return false;
        }

        if (ch == '\n')
        {
            return true;
        }

        if (ch != '\r')
        {
            line.push_back(ch);
        }
    }
}

// 处理一个具体客户端连接：
// 循环读取请求，交给业务层处理，再把结果回写给客户端。
static void handleClient(SOCKET client, int clientNo)
{
    printf("客户端 #%d 已连接\n", clientNo);

    std::string request;
    while (readLine(client, request))
    {
        std::string response = handleClientRequest(request);
        response.push_back('\n');

        if (!sendAll(client, response))
        {
            break;
        }
    }

    closesocket(client);
    printf("客户端 #%d 已断开\n", clientNo);
}

// 服务端入口：
// 初始化控制台和业务数据，启动监听端口，并为每个客户端创建处理线程。
void runServer()
{
    initConsole();
    initServerData();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WinSock 初始化失败\n");
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("创建监听 socket 失败\n");
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8888);

    int reuse = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        printf("绑定 8888 端口失败，请确认端口未被占用\n");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("监听失败\n");
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    printf("服务器已启动，监听端口 8888\n");
    printf("等待客户端连接...\n");

    std::atomic<int> clientCounter = 0;
    while (true)
    {
        // 每接受到一个新连接，就分配一个编号并交给独立线程处理。
        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            printf("接收客户端连接失败\n");
            continue;
        }

        int clientNo = ++clientCounter;
        std::thread(handleClient, client, clientNo).detach();
    }
}
