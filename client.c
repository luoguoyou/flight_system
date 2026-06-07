/*
 * ============================================================
 *   client.c —— 客户端主程序
 *   功能：TCP 连接管理、请求发送、响应接收和解析，
 *         为用户提供完整的航班查询、订票、退票等操作界面。
 *   架构：基于行协议（一行一个数据包）的 C/S 交互模式。
 *   协议：通过 makePacket 构建请求，splitPacket 解析响应。
 * ============================================================
 */
#include "client.h"
#include "protocol.h"
#include "utils.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

/* 响应缓冲区最大长度，需与服务端保持一致 */
#define MAX_RESPONSE 16384

/*
 * TcpClient —— TCP 连接结构体
 * 封装单个 socket 连接的所有状态信息。
 */
typedef struct {
    SOCKET sock;       /* 当前 socket 句柄 */
    int wsaReady;      /* Winsock 库是否已成功初始化 */
} TcpClient;

/*
 * Session —— 用户会话结构体
 * 记录用户登录后的身份信息和会话状态。
 */
typedef struct {
    int loggedIn;              /* 是否已登录 */
    char username[32];         /* 当前登录的用户名 */
    char token[32];            /* 会话令牌（由服务端生成） */
    int role;                  /* 角色：0=管理员，1=普通用户 */
} Session;

/*=================== TCP 底层通信函数 ===================*/

/*
 * tcpClientClose —— 关闭 TCP 连接
 */
static void tcpClientClose(TcpClient* tc) {
    if (tc->sock != INVALID_SOCKET) {
        closesocket(tc->sock);
        tc->sock = INVALID_SOCKET;
    }
}

/*
 * tcpClientConnect —— 连接服务器
 * 创建 TCP socket 并连接本机 8888 端口。
 * 返回值：1=成功，0=失败
 */
static int tcpClientConnect(TcpClient* tc)
{
    tcpClientClose(tc);
    tc->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tc->sock == INVALID_SOCKET) {
        printf("创建 socket 失败\n");
        return 0;
    }

    /* 配置服务器地址：127.0.0.1:8888 */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(tc->sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        tcpClientClose(tc);
        printf("无法连接服务器 127.0.0.1:8888，请先启动服务端\n");
        return 0;
    }
    printf("已连接服务器 127.0.0.1:8888\n");
    return 1;
}

/*
 * tcpClientSendLine —— 发送一行数据
 * 协议要求每个数据包末尾加 \n 以区分不同包。
 * 采用循环发送方式，确保大数据包完整发送。
 */
static int tcpClientSendLine(TcpClient* tc, const char* packet)
{
    int packetLen = (int)strlen(packet);
    int totalLen = packetLen + 1;  /* +1 给末尾的 \n */
    char* buf = (char*)malloc(totalLen + 1);
    if (!buf) return 0;

    memcpy(buf, packet, packetLen);
    buf[packetLen] = '\n';
    buf[totalLen] = '\0';

    /* 循环发送直到全部字节发送完毕 */
    const char* data = buf;
    int left = totalLen;
    while (left > 0) {
        int sent = send(tc->sock, data, left, 0);
        if (sent <= 0) { free(buf); return 0; }
        data += sent;
        left -= sent;
    }
    free(buf);
    return 1;
}

/*
 * tcpClientReadLine —— 从服务器读取一行响应
 * 逐字节读取直到遇到 \n，支持 \r\n 和 \n 两种换行格式。
 */
static int tcpClientReadLine(TcpClient* tc, char* line, int maxLen)
{
    int pos = 0;
    char ch;
    while (pos < maxLen - 1) {
        int received = recv(tc->sock, &ch, 1, 0);
        if (received <= 0) return 0;
        if (ch == '\n') { line[pos] = '\0'; return 1; }
        if (ch != '\r') line[pos++] = ch;
    }
    line[pos] = '\0';
    return 1;
}

/*
 * tcpClientRequest —— 一次完整的请求-应答交互
 *
 * 流程：
 * 1. 用 makePacket 将请求字段组包
 * 2. 发送给服务端
 * 3. 读取服务端返回的完整响应行
 * 4. 遇连接异常自动重连一次
 *
 * 返回值：1=通信成功，0=通信失败
 */
static int tcpClientRequest(TcpClient* tc, const char** fields, int fieldCount,
                            char* response, int respSize)
{
    /* 将请求字段构建为协议数据包 */
    char packet[4096];
    makePacket(fields, fieldCount, packet, sizeof(packet));

    /* 最多尝试 2 次（含重连） */
    for (int attempt = 0; attempt < 2; attempt++) {
        if (tc->sock == INVALID_SOCKET && !tcpClientConnect(tc))
            continue;
        if (tcpClientSendLine(tc, packet) &&
            tcpClientReadLine(tc, response, respSize)) {
            return 1;  /* 发送和接收均成功 */
        }
        printf("连接异常，正在尝试重连...\n");
        tcpClientClose(tc);
    }
    return 0;
}

/*
 * tcpClientInit —— 初始化客户端
 * 初始化 Winsock 库并尝试连接服务器。
 */
static int tcpClientInit(TcpClient* tc)
{
    tc->sock = INVALID_SOCKET;
    tc->wsaReady = 0;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WinSock 初始化失败\n");
        return 0;
    }
    tc->wsaReady = 1;
    return tcpClientConnect(tc);
}

/*
 * tcpClientDestroy —— 释放客户端资源
 */
static void tcpClientDestroy(TcpClient* tc)
{
    tcpClientClose(tc);
    if (tc->wsaReady) WSACleanup();
}

/*=================== 通用工具函数 ===================*/

/*
 * sendRequest —— 发送请求、解析响应并显示结果
 *
 * 核心功能：
 * 1. 调用 tcpClientRequest 完成网络通信
 * 2. 用 splitPacket 解析响应数据包
 * 3. 显示响应内容给用户（字段1 = 正文消息）
 * 4. 根据首字段是 "OK" 还是 "ERR" 判断请求是否成功
 *
 * rawFields/rawCount：可选参数，返回原始拆分字段供调用方使用
 * 返回值：1=请求成功（OK），0=请求失败（ERR）
 */
static int sendRequest(TcpClient* tc, const char** fields, int fieldCount,
                       char rawFields[][PROTOCOL_MAX_FIELD_LEN], int* rawCount)
{
    char response[MAX_RESPONSE];

    /* 步骤1：发送请求并接收响应 */
    if (!tcpClientRequest(tc, fields, fieldCount, response, sizeof(response))) {
        printf("请求失败，服务器不可用\n");
        return 0;
    }

    /* 步骤2：如果调用方需要原始字段，保留一份拆分结果 */
    if (rawFields && rawCount) {
        *rawCount = splitPacket(response, rawFields);
    }

    /* 步骤3：解析并打印响应内容 */
    char parts[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
    int partCount = splitPacket(response, parts);

    if (partCount >= 2) {
        /* 字段0=OK/ERR，字段1=消息正文 */
        printf("\n%s\n", parts[1]);
    } else {
        /* 字段数不足说明数据包格式异常 */
        printf("\n服务器返回格式异常\n");
    }

    /* 步骤4：根据首字段判断结果 */
    return (partCount >= 1 && strcmp(parts[0], "OK") == 0);
}

/* 检查用户是否已登录，未登录则提示 */
static int requireLogin(const Session* session)
{
    if (!session->loggedIn) { printf("请先登录\n"); return 0; }
    return 1;
}

/* 检查是否为管理员账号 */
static int requireAdmin(const Session* session)
{
    if (!requireLogin(session)) return 0;
    if (session->role != 0) { printf("当前账号不是管理员\n"); return 0; }
    return 1;
}

/*
 * readInput —— 带提示的用户输入读取函数
 * 自动去除 fgets 读取时末尾的换行符。
 */
static void readInput(const char* prompt, char* out, int maxLen)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(out, maxLen, stdin)) {
        int len = (int)strlen(out);
        if (len > 0 && out[len-1] == '\n') out[len-1] = '\0';
    } else {
        out[0] = '\0';
    }
}

/*=================== 登录流程 ===================*/

/*
 * loginFlow —— 用户登录流程
 *
 * 功能：
 * 1. 显示登录/注册菜单
 * 2. 用户可选择注册新账号或登录已有账号
 * 3. 登录成功后从响应中提取角色、用户名和会话令牌
 *
 * 登录响应协议：
 * 服务端返回5个字段：OK | "登录成功" | 角色 | 用户名 | 令牌
 * 客户端利用 rawFields 捕获这5个字段以提取身份信息。
 */
static int loginFlow(TcpClient* tc, Session* session)
{
    while (1) {
        char choice[16];
        printf("\n1 注册\n2 登录\n0 退出系统\n请选择：");
        fgets(choice, sizeof(choice), stdin);
        int clen = (int)strlen(choice);
        if (clen > 0 && choice[clen-1] == '\n') choice[clen-1] = '\0';

        /* 注册 */
        if (strcmp(choice, "1") == 0) {
            char user[32], pwd[32];
            readInput("用户名：", user, sizeof(user));
            readInput("密码：", pwd, sizeof(pwd));
            const char* f[] = {"REGISTER", user, pwd};
            sendRequest(tc, f, 3, NULL, NULL);
            continue;
        }

        /* 登录 */
        if (strcmp(choice, "2") == 0) {
            char user[32], pwd[32];
            readInput("用户名：", user, sizeof(user));
            readInput("密码：", pwd, sizeof(pwd));

            /* rawFields 捕获完整响应，从中提取身份信息 */
            char rawFields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
            int rawCount = 0;
            const char* f[] = {"LOGIN", user, pwd};

            if (sendRequest(tc, f, 3, rawFields, &rawCount) && rawCount >= 5) {
                /* fields[0]=OK, fields[1]=消息, fields[2]=角色,
                   fields[3]=用户名, fields[4]=令牌 */
                session->loggedIn = 1;
                session->role = atoi(rawFields[2]);
                strcpy(session->username, rawFields[3]);
                strcpy(session->token, rawFields[4]);
                return 1;  /* 登录成功，进入主菜单 */
            }
            continue;
        }

        if (strcmp(choice, "0") == 0) return 0;
        printf("无效选择\n");
    }
}

/*=================== 业务功能函数 ===================*/

/*
 * 每个业务函数对应一个菜单选项，遵循相同模式：
 * 1. 收集用户输入
 * 2. 构造请求字段数组
 * 3. 调用 sendRequest 发送请求并显示结果
 */

/* 查看所有航班（管理员和普通用户通用） */
static void showFlights(TcpClient* tc) {
    const char* f[] = {"LIST_FLIGHTS"};
    sendRequest(tc, f, 1, NULL, NULL);
}

/* 按目的地搜索航班 */
static void searchFlights(TcpClient* tc) {
    char dest[32];
    readInput("请输入目的地：", dest, sizeof(dest));
    const char* f[] = {"SEARCH_DESTINATION", dest};
    sendRequest(tc, f, 2, NULL, NULL);
}

/*
 * bookTicket —— 订票
 * 如果余票不足且服务端返回"余票不足"信息，
 * 询问用户是否加入候补队列。
 */
static void bookTicket(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;

    char flightNo[20], name[32], phone[32], id[32], ticketStr[16];
    readInput("请输入航班号：", flightNo, sizeof(flightNo));
    readInput("请输入姓名：", name, sizeof(name));
    readInput("请输入电话：", phone, sizeof(phone));
    readInput("请输入身份证号：", id, sizeof(id));
    readInput("请输入订票数量：", ticketStr, sizeof(ticketStr));

    /* 发送订票请求，同时捕获原始响应字段以便判断余票情况 */
    char rawFields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
    int rawCount = 0;
    const char* f[] = {"BOOK", session->token, flightNo, name, phone, id, ticketStr};

    /* sendRequest 返回 1 表示成功，直接返回；失败时检查原因 */
    if (sendRequest(tc, f, 7, rawFields, &rawCount)) return;

    /* 如果是余票不足，询问是否加入候补 */
    if (rawCount >= 2 && strstr(rawFields[1], "余票不足")) {
        char choice[8];
        readInput("是否进入候补队列（1-是 0-否）：", choice, sizeof(choice));
        if (strcmp(choice, "1") == 0) {
            /* 多传一个 "WAIT" 字段表示要求加入候补 */
            const char* f2[] = {"BOOK", session->token, flightNo, name, phone, id, ticketStr, "WAIT"};
            sendRequest(tc, f2, 8, NULL, NULL);
        }
    }
}

/* 退票 */
static void refundTicket(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    char orderId[32];
    readInput("请输入订单号：", orderId, sizeof(orderId));
    const char* f[] = {"REFUND", session->token, orderId};
    sendRequest(tc, f, 3, NULL, NULL);
}

/* 查看所有订单（管理员功能） */
static void showOrderFile(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    const char* f[] = {"SHOW_ORDERS", session->token};
    sendRequest(tc, f, 2, NULL, NULL);
}

/* 查看候补队列 */
static void showWaitQueueView(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    const char* f[] = {"SHOW_WAITLIST", session->token};
    sendRequest(tc, f, 2, NULL, NULL);
}

/* 新增航班（管理员功能） */
static void addFlight(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;

    char fn[20], start[32], dest[32], date[20], st[16], at[16], pr[16], seat[16];
    readInput("航班号：", fn, sizeof(fn));
    readInput("出发地：", start, sizeof(start));
    readInput("目的地：", dest, sizeof(dest));
    readInput("日期：", date, sizeof(date));
    readInput("起飞时间：", st, sizeof(st));
    readInput("到达时间：", at, sizeof(at));
    readInput("票价：", pr, sizeof(pr));
    readInput("总座位：", seat, sizeof(seat));

    const char* f[] = {"ADD_FLIGHT", session->token, fn, start, dest, date, st, at, pr, seat};
    sendRequest(tc, f, 10, NULL, NULL);
}

/* 删除航班（管理员功能） */
static void deleteFlight(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;
    char fn[20];
    readInput("输入航班号：", fn, sizeof(fn));
    const char* f[] = {"DELETE_FLIGHT", session->token, fn};
    sendRequest(tc, f, 3, NULL, NULL);
}

/* 修改航班票价（管理员功能） */
static void updateFlight(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;
    char fn[20], pr[16];
    readInput("请输入航班号：", fn, sizeof(fn));
    readInput("输入新票价：", pr, sizeof(pr));
    const char* f[] = {"UPDATE_FLIGHT", session->token, fn, pr};
    sendRequest(tc, f, 4, NULL, NULL);
}

/* 查看所有乘客信息（管理员功能） */
static void showPassengers(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;
    const char* f[] = {"SHOW_PASSENGERS", session->token};
    sendRequest(tc, f, 2, NULL, NULL);
}

/*=================== 主流程 ===================*/

/*
 * runClient —— 客户端主入口
 *
 * 完整流程：
 * 1. initConsole() —— 设置控制台 UTF-8 编码
 * 2. tcpClientInit() —— 初始化 Winsock 并连接服务器
 * 3. loginFlow() —— 登录/注册流程
 * 4. 主循环 —— 根据角色显示不同菜单，处理用户选择
 *    管理员：查看/搜索/新增/删除/修改航班 + 乘客/订单/候补管理
 *    普通用户：查看/搜索航班 + 订票/退票 + 查看订单/候补
 * 5. 选择"退出"时发送 LOGOUT 请求，断开连接
 */
void runClient()
{
    initConsole();

    /* 初始化网络连接 */
    TcpClient tc;
    if (!tcpClientInit(&tc)) return;

    /* 登录 */
    Session session;
    memset(&session, 0, sizeof(session));
    if (!loginFlow(&tc, &session)) {
        tcpClientDestroy(&tc);
        return;
    }

    /* 主菜单循环 */
    while (1) {
        char choice[16];

        /* ===== 管理员菜单 ===== */
        if (session.role == 0) {
            adminMenu();
            fgets(choice, sizeof(choice), stdin);
            char* found = strchr(choice, '\n');
            if (found) *found = '\0';

            if (strcmp(choice, "1") == 0) showFlights(&tc);
            else if (strcmp(choice, "2") == 0) searchFlights(&tc);
            else if (strcmp(choice, "3") == 0) addFlight(&tc, &session);
            else if (strcmp(choice, "4") == 0) deleteFlight(&tc, &session);
            else if (strcmp(choice, "5") == 0) updateFlight(&tc, &session);
            else if (strcmp(choice, "6") == 0) showPassengers(&tc, &session);
            else if (strcmp(choice, "7") == 0) showOrderFile(&tc, &session);
            else if (strcmp(choice, "8") == 0) showWaitQueueView(&tc, &session);
            else if (strcmp(choice, "0") == 0) {
                const char* f[] = {"LOGOUT", session.token};
                char resp[128];
                tcpClientRequest(&tc, f, 2, resp, sizeof(resp));
                break;
            }
            else printf("无效选择\n");
            continue;
        }

        /* ===== 普通用户菜单 ===== */
        userMenu();
        fgets(choice, sizeof(choice), stdin);
        char* found = strchr(choice, '\n');
        if (found) *found = '\0';

        if (strcmp(choice, "1") == 0) showFlights(&tc);
        else if (strcmp(choice, "2") == 0) searchFlights(&tc);
        else if (strcmp(choice, "3") == 0) bookTicket(&tc, &session);
        else if (strcmp(choice, "4") == 0) refundTicket(&tc, &session);
        else if (strcmp(choice, "5") == 0) showOrderFile(&tc, &session);
        else if (strcmp(choice, "6") == 0) showWaitQueueView(&tc, &session);
        else if (strcmp(choice, "0") == 0) {
            const char* f[] = {"LOGOUT", session.token};
            char resp[128];
            tcpClientRequest(&tc, f, 2, resp, sizeof(resp));
            break;
        }
        else printf("无效选择\n");
    }

    tcpClientDestroy(&tc);
}
