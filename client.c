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

#define MAX_RESPONSE 16384

typedef struct {
    SOCKET sock;
    int wsaReady;
} TcpClient;

static void tcpClientClose(TcpClient* tc) {
    if (tc->sock != INVALID_SOCKET) {
        closesocket(tc->sock);
        tc->sock = INVALID_SOCKET;
    }
}

static int tcpClientConnect(TcpClient* tc)
{
    tcpClientClose(tc);
    tc->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tc->sock == INVALID_SOCKET) {
        printf("创建 socket 失败\n");
        return 0;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(tc->sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        tcpClientClose(tc);
        printf("无法连接服务器 127.0.0.1:8888\xa3\xac请先启动服务端\n");
        return 0;
    }
    printf("已连接服务器 127.0.0.1:8888\n");
    return 1;
}

static int tcpClientSendLine(TcpClient* tc, const char* packet)
{
    int packetLen = (int)strlen(packet);
    int totalLen = packetLen + 1;
    char* buf = (char*)malloc(totalLen + 1);
    if (!buf) return 0;
    memcpy(buf, packet, packetLen);
    buf[packetLen] = '\n';
    buf[totalLen] = '\0';
    const char* data = buf;
    int left = totalLen;
    while (left > 0) {
        int sent = send(tc->sock, data, left, 0);
        if (sent <= 0) { free(buf); return 0; }
        data += sent; left -= sent;
    }
    free(buf);
    return 1;
}

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

static int tcpClientRequest(TcpClient* tc, const char** fields, int fieldCount, char* response, int respSize)
{
    char packet[4096];
    makePacket(fields, fieldCount, packet, sizeof(packet));

    for (int attempt = 0; attempt < 2; attempt++) {
        if (tc->sock == INVALID_SOCKET && !tcpClientConnect(tc)) continue;
        if (tcpClientSendLine(tc, packet) && tcpClientReadLine(tc, response, respSize)) {
            return 1;
        }
        printf("连接异常，正在尝试重连...\n");
        tcpClientClose(tc);
    }
    return 0;
}

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

static void tcpClientDestroy(TcpClient* tc)
{
    tcpClientClose(tc);
    if (tc->wsaReady) WSACleanup();
}

typedef struct {
    int loggedIn;
    char username[32];
    char token[32];
    int role;
} Session;

static int sendRequest(TcpClient* tc, const char** fields, int fieldCount,
                       char rawFields[][PROTOCOL_MAX_FIELD_LEN], int* rawCount)
{
    char response[MAX_RESPONSE];
    if (!tcpClientRequest(tc, fields, fieldCount, response, sizeof(response))) {
        printf("请求失败，服务器不可用\n");
        return 0;
    }

    if (rawFields && rawCount) {
        *rawCount = splitPacket(response, rawFields);
    }

    char parts[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
    int partCount = splitPacket(response, parts);
    if (partCount >= 2) {
        printf("\n%s\n", parts[1]);
    } else {
        printf("\n服务器返回格式异常\n");
    }

    return (partCount >= 1 && strcmp(parts[0], "OK") == 0);
}

static int requireLogin(const Session* session)
{
    if (!session->loggedIn) { printf("请先登录\n"); return 0; }
    return 1;
}

static int requireAdmin(const Session* session)
{
    if (!requireLogin(session)) return 0;
    if (session->role != 0) { printf("当前账号不是管理员\n"); return 0; }
    return 1;
}

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

static int loginFlow(TcpClient* tc, Session* session)
{
    while (1) {
        char choice[16];
        printf("\n1 注册\n2 登录\n0 退出系统\n请选择：");
        fgets(choice, sizeof(choice), stdin);
        int clen = (int)strlen(choice);
        if (clen > 0 && choice[clen-1] == '\n') choice[clen-1] = '\0';

        if (strcmp(choice, "1") == 0) {
            char user[32], pwd[32];
            readInput("用户名：", user, sizeof(user));
            readInput("密码：", pwd, sizeof(pwd));
            const char* f[] = {"REGISTER", user, pwd};
            sendRequest(tc, f, 3, NULL, NULL);
            continue;
        }
        if (strcmp(choice, "2") == 0) {
            char user[32], pwd[32];
            readInput("用户名：", user, sizeof(user));
            readInput("密码：", pwd, sizeof(pwd));
            char rawFields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
            int rawCount = 0;
            const char* f[] = {"LOGIN", user, pwd};
            if (sendRequest(tc, f, 3, rawFields, &rawCount) && rawCount >= 5) {
                session->loggedIn = 1;
                session->role = atoi(rawFields[2]);
                strcpy(session->username, rawFields[3]);
                strcpy(session->token, rawFields[4]);
                return 1;
            }
            continue;
        }
        if (strcmp(choice, "0") == 0) return 0;
        printf("无效选择\n");
    }
}



static void showFlights(TcpClient* tc) {
    const char* f[] = {"LIST_FLIGHTS"};
    sendRequest(tc, f, 1, NULL, NULL);
}

static void searchFlights(TcpClient* tc) {
    char dest[32];
    readInput("请输入目的地：", dest, sizeof(dest));
    const char* f[] = {"SEARCH_DESTINATION", dest};
    sendRequest(tc, f, 2, NULL, NULL);
}

static void bookTicket(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    char flightNo[20], name[32], phone[32], id[32], ticketStr[16];
    readInput("请输入航班号：", flightNo, sizeof(flightNo));
    readInput("请输入姓名：", name, sizeof(name));
    readInput("请输入电话：", phone, sizeof(phone));
    readInput("请输入身份证号：", id, sizeof(id));
    readInput("请输入订票数量：", ticketStr, sizeof(ticketStr));

    char rawFields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
    int rawCount = 0;
    const char* f[] = {"BOOK", session->token, flightNo, name, phone, id, ticketStr};
    if (sendRequest(tc, f, 7, rawFields, &rawCount)) return;

    if (rawCount >= 2 && strstr(rawFields[1], "余票不足")) {
        char choice[8];
        readInput("是否进入候补队列（1-是 0-否）：", choice, sizeof(choice));
        if (strcmp(choice, "1") == 0) {
            const char* f2[] = {"BOOK", session->token, flightNo, name, phone, id, ticketStr, "WAIT"};
            sendRequest(tc, f2, 8, NULL, NULL);
        }
    }
}

static void refundTicket(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    char orderId[32];
    readInput("请输入订单号：", orderId, sizeof(orderId));
    const char* f[] = {"REFUND", session->token, orderId};
    sendRequest(tc, f, 3, NULL, NULL);
}

static void showOrderFile(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    const char* f[] = {"SHOW_ORDERS", session->token};
    sendRequest(tc, f, 2, NULL, NULL);
}

static void showWaitQueueView(TcpClient* tc, const Session* session) {
    if (!requireLogin(session)) return;
    const char* f[] = {"SHOW_WAITLIST", session->token};
    sendRequest(tc, f, 2, NULL, NULL);
}

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

static void deleteFlight(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;
    char fn[20];
    readInput("输入航班号：", fn, sizeof(fn));
    const char* f[] = {"DELETE_FLIGHT", session->token, fn};
    sendRequest(tc, f, 3, NULL, NULL);
}

static void updateFlight(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;
    char fn[20], pr[16];
    readInput("请输入航班号：", fn, sizeof(fn));
    readInput("输入新票价：", pr, sizeof(pr));
    const char* f[] = {"UPDATE_FLIGHT", session->token, fn, pr};
    sendRequest(tc, f, 4, NULL, NULL);
}

static void showPassengers(TcpClient* tc, const Session* session) {
    if (!requireAdmin(session)) return;
    const char* f[] = {"SHOW_PASSENGERS", session->token};
    sendRequest(tc, f, 2, NULL, NULL);
}

void runClient()
{
    initConsole();

    TcpClient tc;
    if (!tcpClientInit(&tc)) return;

    Session session;
    memset(&session, 0, sizeof(session));
    if (!loginFlow(&tc, &session)) {
        tcpClientDestroy(&tc);
        return;
    }

    while (1) {
        char choice[16];

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
