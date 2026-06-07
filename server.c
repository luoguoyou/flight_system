#include "server.h"
#include "service.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define MAX_RESPONSE 16384

typedef struct {
    SOCKET client;
    int clientNo;
} ClientArgs;

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

static DWORD WINAPI handleClient(LPVOID param)
{
    ClientArgs* args = (ClientArgs*)param;
    SOCKET client = args->client;
    int clientNo = args->clientNo;
    free(args);

    printf("客户端 #%d 已连接\n", clientNo);

    char request[4096];
    char response[MAX_RESPONSE];

    while (readLine(client, request, sizeof(request))) {
        handleClientRequest(request, response, sizeof(response));
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

void runServer()
{
    initConsole();
    initServerData();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WinSock 初始化失败\n");
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("创建监听 socket 失败\n");
        WSACleanup();
        return;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8888);

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

    while (1) {
        SOCKET client = accept(listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            printf("接收客户端连接失败\n");
            continue;
        }

        LONG clientNo = InterlockedIncrement(&clientCounter);
        ClientArgs* args = (ClientArgs*)malloc(sizeof(ClientArgs));
        args->client = client;
        args->clientNo = (int)clientNo;

        HANDLE hThread = CreateThread(NULL, 0, handleClient, args, 0, NULL);
        if (hThread) CloseHandle(hThread);
        else {
            printf("创建线程失败\n");
            closesocket(client);
            free(args);
        }
    }

    closesocket(listenSocket);
    WSACleanup();
}