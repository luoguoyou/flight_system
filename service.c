#include "service.h"
#include "booking.h"
#include "flight_ops.h"
#include "globals.h"
#include "protocol.h"
#include "user.h"
#include "utils.h"
#include "waitlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_SESSION 50
#define MAX_ORDERS 500
#define MAX_RESPONSE 16384
#define MAX_ORDER_FIELDS 20

// Mutex for data consistency across threads
static CRITICAL_SECTION g_dataMutex;

// Session table (parallel arrays replacing unordered_map)
static char g_sessionTokens[MAX_SESSION][32];
static char g_sessionUsers[MAX_SESSION][32];
static int g_sessionCount = 0;
static int g_nextSessionNumber = 1;

// Lightweight in-memory order record
typedef struct {
    char orderId[32];
    char flightNo[32];
    char name[32];
    char phone[32];
    char id[32];
    int ticketNum;
} OrderRecord;

// Helper: pack OK response
static void packOk(const char* body, char* out, int outSize)
{
    const char* fields[2] = {"OK", body};
    makePacket(fields, 2, out, outSize);
}

// Helper: pack ERR response
static void packFail(const char* body, char* out, int outSize)
{
    const char* fields[2] = {"ERR", body};
    makePacket(fields, 2, out, outSize);
}

static int findUser(const char* username)
{
    for (int i = 0; i < userCount; i++) {
        if (strcmp(username, users[i].username) == 0) return i;
    }
    return -1;
}

static int isPositiveInt(const char* text, int* value)
{
    if (!text || !*text) return 0;
    for (const char* p = text; *p; p++) {
        if (*p < '0' || *p > '9') return 0;
    }
    *value = atoi(text);
    return *value > 0;
}

static int isPositiveFloat(const char* text, float* value)
{
    if (!text || !*text) return 0;
    char* endPtr = NULL;
    *value = (float)strtod(text, &endPtr);
    return endPtr && *endPtr == '\0' && *value > 0;
}

// Create a session token for logged-in user
static void createSession(const char* username, char* tokenOut)
{
    sprintf(tokenOut, "S%d", g_nextSessionNumber++);
    strcpy(g_sessionTokens[g_sessionCount], tokenOut);
    strcpy(g_sessionUsers[g_sessionCount], username);
    g_sessionCount++;
}

// Look up session token to get username, returns 1 if found
static int getSessionUser(const char* token, char* usernameOut)
{
    for (int i = 0; i < g_sessionCount; i++) {
        if (strcmp(g_sessionTokens[i], token) == 0) {
            strcpy(usernameOut, g_sessionUsers[i]);
            return 1;
        }
    }
    return 0;
}

// Check if token belongs to an admin account
static int requireAdminByToken(const char* token, char* usernameOut)
{
    if (!getSessionUser(token, usernameOut)) return 0;
    int idx = findUser(usernameOut);
    return (idx != -1 && users[idx].role == 0);
}

static void saveUsersToFile()
{
    FILE* fp = fopen("user.txt", "w");
    if (!fp) return;
    for (int i = 0; i < userCount; i++) {
        fprintf(fp, "%s %s %d\n", users[i].username, users[i].password, users[i].role);
    }
    fclose(fp);
}

// Load all orders from passenger.txt, returns count
static int loadOrders(OrderRecord* orders, int maxOrders)
{
    FILE* fp = fopen("passenger.txt", "r");
    if (!fp) return 0;
    int count = 0;
    while (count < maxOrders && fscanf(fp, "%19s%19s%19s%19s%19s%d",
           orders[count].orderId, orders[count].flightNo,
           orders[count].name, orders[count].phone,
           orders[count].id, &orders[count].ticketNum) != EOF)
    {
        stripBom(orders[count].orderId);
        count++;
    }
    fclose(fp);
    return count;
}

// Save all orders back to passenger.txt
static int saveOrders(const OrderRecord* orders, int count)
{
    FILE* fp = fopen("passenger.txt", "w");
    if (!fp) return 0;
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s %s %s %s %s %d\n",
            orders[i].orderId, orders[i].flightNo,
            orders[i].name, orders[i].phone,
            orders[i].id, orders[i].ticketNum);
    }
    fclose(fp);
    return 1;
}

// Formatting helpers
static void appendFlightHeader(char** p, char* end)
{
    *p += snprintf(*p, end - *p,
        "====================================================================================================================\n");
    *p += snprintf(*p, end - *p,
        "%-10s %-8s %-8s %-12s %-10s %-10s %-10s %-10s %-10s\n",
        "航班号", "出发地", "目的地", "日期", "起飞", "到达", "票价", "总座位", "余票");
    *p += snprintf(*p, end - *p,
        "====================================================================================================================\n");
}

static void formatFlightRow(const Flight* f, char* line, int lineSize)
{
    snprintf(line, lineSize, "%-10s %-8s %-8s %-12s %-10s %-10s %-10.2f %-10d %-10d",
        f->flightNo, f->start, f->destination, f->date, f->startTime,
        f->arriveTime, f->price, f->totalSeat, f->remainSeat);
}

static void formatOrderBrief(const OrderRecord* o, char* line, int lineSize)
{
    snprintf(line, lineSize, "%-10s %-10s %-10s %-15s %-20s %-6d",
        o->orderId, o->flightNo, o->name, o->phone, o->id, o->ticketNum);
}

static void formatOrderDetail(const OrderRecord* o, char* out, int outSize)
{
    int pos = findFlight((char*)o->flightNo);
    if (pos != -1) {
        float total = flight[pos].price * o->ticketNum;
        snprintf(out, outSize, "订单号：%s 航班号：%s 姓名：%s 电话：%s 身份证：%s 票数：%d 航线：%s->%s 日期：%s 起飞：%s 总金额：%.2f",
            o->orderId, o->flightNo, o->name, o->phone, o->id, o->ticketNum,
            flight[pos].start, flight[pos].destination, flight[pos].date, flight[pos].startTime, total);
    } else {
        snprintf(out, outSize, "订单号：%s 航班号：%s 姓名：%s 电话：%s 身份证：%s 票数：%d",
            o->orderId, o->flightNo, o->name, o->phone, o->id, o->ticketNum);
    }
}

// Auto-confirm waiting passenger when seats become available
static void confirmWaitingPassenger(int flightPos, const WaitingPassenger* w)
{
    Passenger passenger;
    generateOrderId(passenger.orderId);
    strcpy(passenger.name, w->name);
    strcpy(passenger.phone, w->phone);
    strcpy(passenger.id, w->id);
    passenger.ticketNum = w->ticketNum;
    passenger.next = NULL;
    flight[flightPos].remainSeat -= passenger.ticketNum;
    savePassengerToFile(&passenger, flight[flightPos].flightNo);
}

static void processWaitingList(int flightPos)
{
    while (1) {
        int waitIdx = findFirstWaitingIndex(flight[flightPos].flightNo);
        if (waitIdx == -1) break;
        if (flight[flightPos].remainSeat < waitQueue[waitIdx].ticketNum) break;
        WaitingPassenger wp;
        if (!removeWaitingAt(waitIdx, &wp)) break;
        confirmWaitingPassenger(flightPos, &wp);
    }
    saveWaitQueue();
}

// ======== Handler Functions ========

static void handleLogout(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount >= 2) {
        for (int i = 0; i < g_sessionCount; i++) {
            if (strcmp(g_sessionTokens[i], fields[1]) == 0) {
                for (int j = i; j < g_sessionCount - 1; j++) {
                    strcpy(g_sessionTokens[j], g_sessionTokens[j+1]);
                    strcpy(g_sessionUsers[j], g_sessionUsers[j+1]);
                }
                g_sessionCount--;
                break;
            }
        }
    }
    packOk("已退出登录", out, outSize);
}

static void handleRegister(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 3) { packFail("注册参数不足", out, outSize); return; }
    if (!fields[1][0] || !fields[2][0]) {
        packFail("用户名和密码不能为空", out, outSize); return;
    }
    if (findUser(fields[1]) != -1) {
        packFail("用户名已存在，请更换用户名", out, outSize); return;
    }
    if (userCount >= MAX_USER) {
        packFail("用户容量已满，无法注册", out, outSize); return;
    }
    strcpy(users[userCount].username, fields[1]);
    strcpy(users[userCount].password, fields[2]);
    users[userCount].role = 1;
    userCount++;
    saveUsersToFile();
    packOk("注册成功，请登录", out, outSize);
}

static void handleLogin(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 3) { packFail("登录参数不足", out, outSize); return; }
    int idx = findUser(fields[1]);
    if (idx == -1 || strcmp(fields[2], users[idx].password) != 0) {
        packFail("账号或密码错误", out, outSize); return;
    }
    char token[32];
    createSession(fields[1], token);
    char roleStr[4];
    sprintf(roleStr, "%d", users[idx].role);
    const char* payload[5] = {"OK", "登录成功", roleStr, fields[1], token};
    makePacket(payload, 5, out, outSize);
}

static void handleListFlights(char* out, int outSize)
{
    if (flightCount == 0) { packOk("暂无航班数据", out, outSize); return; }
    char body[MAX_RESPONSE];
    char* p = body;
    char* end = body + sizeof(body);
    appendFlightHeader(&p, end);
    char row[256];
    for (int i = 0; i < flightCount; i++) {
        formatFlightRow(&flight[i], row, sizeof(row));
        p += snprintf(p, end - p, "%s\n", row);
    }
    p += snprintf(p, end - p, "====================================================================================================================");
    packOk(body, out, outSize);
}

static void handleSearchDestination(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 2 || !fields[1][0]) {
        packFail("请输入目的地", out, outSize); return;
    }
    char body[MAX_RESPONSE];
    char* p = body;
    char* end = body + sizeof(body);
    int matched = 0;
    appendFlightHeader(&p, end);
    char row[256];
    for (int i = 0; i < flightCount; i++) {
        if (strstr(flight[i].destination, fields[1]) != NULL) {
            formatFlightRow(&flight[i], row, sizeof(row));
            p += snprintf(p, end - p, "%s\n", row);
            matched++;
        }
    }
    if (matched == 0) {
        packOk("未查询到符合条件的航班", out, outSize); return;
    }
    p += snprintf(p, end - p, "====================================================================================================================");
    packOk(body, out, outSize);
}

static void handleBookTicket(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 7) { packFail("订票参数不足", out, outSize); return; }
    char username[32];
    if (!getSessionUser(fields[1], username)) {
        packFail("登录状态无效，请重新登录", out, outSize); return;
    }
    int pos = findFlight((char*)fields[2]);
    if (pos == -1) { packFail("航班不存在", out, outSize); return; }
    int ticketNum = 0;
    if (!isPositiveInt(fields[6], &ticketNum)) {
        packFail("订票数量必须为正整数", out, outSize); return;
    }
    if (strlen(fields[3]) == 0 || strlen(fields[4]) == 0 || strlen(fields[5]) == 0) {
        packFail("姓名、电话、身份证不能为空", out, outSize); return;
    }

    if (flight[pos].remainSeat < ticketNum) {
        if (fieldCount >= 8 && strcmp(fields[7], "WAIT") == 0) {
            Passenger passenger;
            strcpy(passenger.name, fields[3]);
            strcpy(passenger.phone, fields[4]);
            strcpy(passenger.id, fields[5]);
            passenger.ticketNum = ticketNum;
            passenger.next = NULL;
            if (!enqueueWaitPassenger(&passenger, flight[pos].flightNo)) {
                packFail("候补队列已满，加入失败", out, outSize); return;
            }
            saveWaitQueue();
            packOk("余票不足，已加入候补队列", out, outSize); return;
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "余票不足，当前余票：%d", flight[pos].remainSeat);
        packFail(buf, out, outSize); return;
    }

    Passenger passenger;
    generateOrderId(passenger.orderId);
    strcpy(passenger.name, fields[3]);
    strcpy(passenger.phone, fields[4]);
    strcpy(passenger.id, fields[5]);
    passenger.ticketNum = ticketNum;
    passenger.next = NULL;
    flight[pos].remainSeat -= ticketNum;
    savePassengerToFile(&passenger, flight[pos].flightNo);
    saveFlight();

    char buf[2048];
    float total = flight[pos].price * ticketNum;
    snprintf(buf, sizeof(buf),
        "====================================\n"
        "               订票成功\n"
        "====================================\n"
        "订单编号：%s\n航班号：%s\n航线：%s -> %s\n日期：%s\n起飞时间：%s\n"
        "乘客姓名：%s\n联系电话：%s\n身份证号：%s\n票价：%.2f 元\n购买数量：%d 张\n"
        "总金额：%.2f 元\n剩余票数：%d 张\n====================================",
        passenger.orderId, flight[pos].flightNo,
        flight[pos].start, flight[pos].destination,
        flight[pos].date, flight[pos].startTime,
        passenger.name, passenger.phone, passenger.id,
        flight[pos].price, ticketNum, total, flight[pos].remainSeat);
    packOk(buf, out, outSize);
}

static void handleRefundTicket(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 3) { packFail("退票参数不足", out, outSize); return; }
    char username[32];
    if (!getSessionUser(fields[1], username)) {
        packFail("登录状态无效，请重新登录", out, outSize); return;
    }

    OrderRecord orders[MAX_ORDERS];
    int orderCount = loadOrders(orders, MAX_ORDERS);
    int found = 0;
    OrderRecord removed;
    for (int i = 0; i < orderCount; i++) {
        if (strcmp(orders[i].orderId, fields[2]) == 0) {
            removed = orders[i];
            for (int j = i; j < orderCount - 1; j++) orders[j] = orders[j+1];
            orderCount--;
            found = 1;
            break;
        }
    }
    if (!found) { packFail("未找到该订单号", out, outSize); return; }

    int pos = findFlight(removed.flightNo);
    if (pos != -1) {
        flight[pos].remainSeat += removed.ticketNum;
        if (flight[pos].remainSeat > flight[pos].totalSeat)
            flight[pos].remainSeat = flight[pos].totalSeat;
    }
    saveOrders(orders, orderCount);
    if (pos != -1) { processWaitingList(pos); }
    saveFlight();

    char buf[1024];
    if (pos != -1) {
        snprintf(buf, sizeof(buf),
            "====================================\n                退票成功\n====================================\n"
            "订单编号：%s\n乘客姓名：%s\n航班号：%s\n退票数量：%d 张\n当前余票：%d 张\n====================================",
            removed.orderId, removed.name, removed.flightNo, removed.ticketNum, flight[pos].remainSeat);
    } else {
        snprintf(buf, sizeof(buf), "退票成功");
    }
    packOk(buf, out, outSize);
}

static void handleChangeTicket(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 4) { packFail("改签参数不足", out, outSize); return; }
    char username[32];
    if (!getSessionUser(fields[1], username)) {
        packFail("登录状态无效，请重新登录", out, outSize); return;
    }

    OrderRecord orders[MAX_ORDERS];
    int orderCount = loadOrders(orders, MAX_ORDERS);
    int orderIdx = -1;
    for (int i = 0; i < orderCount; i++) {
        if (strcmp(orders[i].orderId, fields[2]) == 0) { orderIdx = i; break; }
    }
    if (orderIdx == -1) { packFail("未找到该订单号", out, outSize); return; }

    int oldFlight = findFlight(orders[orderIdx].flightNo);
    int newFlight = findFlight((char*)fields[3]);
    if (newFlight == -1) { packFail("目标航班不存在", out, outSize); return; }
    if (strcmp(orders[orderIdx].flightNo, fields[3]) == 0) {
        packFail("目标航班与原航班相同", out, outSize); return;
    }
    if (flight[newFlight].remainSeat < orders[orderIdx].ticketNum) {
        char buf[128];
        snprintf(buf, sizeof(buf), "目标航班余票不足，当前余票：%d", flight[newFlight].remainSeat);
        packFail(buf, out, outSize); return;
    }

    if (oldFlight != -1) {
        flight[oldFlight].remainSeat += orders[orderIdx].ticketNum;
        if (flight[oldFlight].remainSeat > flight[oldFlight].totalSeat)
            flight[oldFlight].remainSeat = flight[oldFlight].totalSeat;
        processWaitingList(oldFlight);
    }
    flight[newFlight].remainSeat -= orders[orderIdx].ticketNum;
    strcpy(orders[orderIdx].flightNo, fields[3]);
    saveOrders(orders, orderCount);
    saveFlight();

    char detail[MAX_RESPONSE];
    formatOrderDetail(&orders[orderIdx], detail, sizeof(detail));
    char buf[MAX_RESPONSE];
    snprintf(buf, sizeof(buf), "改签成功\n%s", detail);
    packOk(buf, out, outSize);
}

static void handleShowOrders(const char** fields, int fieldCount, char* out, int outSize)
{
    char username[32];
    if (!getSessionUser(fields[1], username)) {
        packFail("登录状态无效，请重新登录", out, outSize); return;
    }

    OrderRecord orders[MAX_ORDERS];
    int orderCount = loadOrders(orders, MAX_ORDERS);
    if (orderCount == 0) { packOk("暂无订单记录", out, outSize); return; }

    char body[MAX_RESPONSE];
    char* p = body;
    char* end = body + sizeof(body);
    p += snprintf(p, end - p, "================================================================================\n");
    p += snprintf(p, end - p, "%-10s %-10s %-10s %-15s %-20s %-6s\n",
        "订单号", "航班号", "姓名", "电话", "身份证", "票数");
    p += snprintf(p, end - p, "================================================================================\n");
    char row[256];
    for (int i = 0; i < orderCount; i++) {
        formatOrderBrief(&orders[i], row, sizeof(row));
        p += snprintf(p, end - p, "%s\n", row);
    }
    p += snprintf(p, end - p, "================================================================================");
    packOk(body, out, outSize);
}

static void handleUserOrders(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 2) { packFail("查询订单参数不足", out, outSize); return; }
    char username[32];
    if (!getSessionUser(fields[1], username)) {
        packFail("登录状态无效，请重新登录", out, outSize); return;
    }

    OrderRecord orders[MAX_ORDERS];
    int orderCount = loadOrders(orders, MAX_ORDERS);
    char body[MAX_RESPONSE];
    char* p = body;
    char* end = body + sizeof(body);
    int matched = 0;
    char detail[MAX_RESPONSE];
    for (int i = 0; i < orderCount; i++) {
        if (strcmp(orders[i].name, username) == 0) {
            formatOrderDetail(&orders[i], detail, sizeof(detail));
            p += snprintf(p, end - p, "%s\n\n", detail);
            matched++;
        }
    }
    if (matched == 0) { packOk("暂无订单", out, outSize); return; }
    packOk(body, out, outSize);
}

static void handleShowWaitQueue(const char** fields, int fieldCount, char* out, int outSize)
{
    char username[32];
    if (!getSessionUser(fields[1], username)) {
        packFail("登录状态无效，请重新登录", out, outSize); return;
    }
    if (front == rear) { packOk("当前暂无候补乘客", out, outSize); return; }

    char body[MAX_RESPONSE];
    char* p = body;
    char* end = body + sizeof(body);
    p += snprintf(p, end - p, "================================================================================\n");
    p += snprintf(p, end - p, "%-6s %-10s %-10s %-15s %-20s %-6s\n",
        "序号", "航班号", "姓名", "电话", "身份证", "票数");
    p += snprintf(p, end - p, "================================================================================\n");
    for (int i = front; i < rear; i++) {
        p += snprintf(p, end - p, "%-6d %-10s %-10s %-15s %-20s %-6d\n",
            i - front + 1, waitQueue[i].flightNo, waitQueue[i].name,
            waitQueue[i].phone, waitQueue[i].id, waitQueue[i].ticketNum);
    }
    p += snprintf(p, end - p, "================================================================================");
    packOk(body, out, outSize);
}

static void handleShowPassengers(const char** fields, int fieldCount, char* out, int outSize)
{
    char username[32];
    if (!requireAdminByToken(fields[1], username)) {
        packFail("无管理员权限", out, outSize); return;
    }

    OrderRecord orders[MAX_ORDERS];
    int orderCount = loadOrders(orders, MAX_ORDERS);
    char body[MAX_RESPONSE];
    char* p = body;
    char* end = body + sizeof(body);
    p += snprintf(p, end - p, "====================== 航班及订票客户信息 ======================\n");
    for (int i = 0; i < flightCount; i++) {
        int hasPassenger = 0;
        p += snprintf(p, end - p, "\n==========================================================\n");
        p += snprintf(p, end - p, "航班号：%s  日期：%s\n", flight[i].flightNo, flight[i].date);
        p += snprintf(p, end - p, "出发地：%s  目的地：%s\n", flight[i].start, flight[i].destination);
        p += snprintf(p, end - p, "起飞时间：%s  到达时间：%s\n", flight[i].startTime, flight[i].arriveTime);
        p += snprintf(p, end - p, "总座位：%d  剩余票数：%d\n", flight[i].totalSeat, flight[i].remainSeat);
        p += snprintf(p, end - p, "----------------------------------------------------------\n");
        for (int j = 0; j < orderCount; j++) {
            if (strcmp(orders[j].flightNo, flight[i].flightNo) == 0) {
                if (!hasPassenger) {
                    p += snprintf(p, end - p, "%-12s %-10s %-15s %-20s %-8s\n",
                        "订单号", "姓名", "电话", "身份证", "票数");
                    p += snprintf(p, end - p, "==========================================================\n");
                    hasPassenger = 1;
                }
                p += snprintf(p, end - p, "%-12s %-10s %-15s %-20s %-8d\n",
                    orders[j].orderId, orders[j].name, orders[j].phone,
                    orders[j].id, orders[j].ticketNum);
            }
        }
        if (!hasPassenger)
            p += snprintf(p, end - p, "暂无订票客户\n");
    }
    p += snprintf(p, end - p, "\n====================== 信息显示完毕 ======================");
    packOk(body, out, outSize);
}

static void handleAddFlight(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 10) { packFail("新增航班参数不足", out, outSize); return; }
    char username[32];
    if (!requireAdminByToken(fields[1], username)) {
        packFail("无管理员权限", out, outSize); return;
    }
    if (flightCount >= MAX_FLIGHT) { packFail("航班数量已满", out, outSize); return; }
    if (findFlight((char*)fields[2]) != -1) { packFail("航班号已存在", out, outSize); return; }

    float price = 0; int totalSeat = 0;
    if (!isPositiveFloat(fields[8], &price) || !isPositiveInt(fields[9], &totalSeat)) {
        packFail("票价或总座位输入无效", out, outSize); return;
    }

    Flight* f = &flight[flightCount];
    strcpy(f->flightNo, fields[2]);
    strcpy(f->start, fields[3]);
    strcpy(f->destination, fields[4]);
    strcpy(f->date, fields[5]);
    strcpy(f->startTime, fields[6]);
    strcpy(f->arriveTime, fields[7]);
    f->price = price;
    f->totalSeat = totalSeat;
    f->remainSeat = totalSeat;
    f->plist = NULL;
    flightCount++;
    saveFlight();
    packOk("新增成功", out, outSize);
}

static void handleDeleteFlight(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 3) { packFail("删除航班参数不足", out, outSize); return; }
    char username[32];
    if (!requireAdminByToken(fields[1], username)) {
        packFail("无管理员权限", out, outSize); return;
    }
    int pos = findFlight((char*)fields[2]);
    if (pos == -1) { packFail("航班不存在", out, outSize); return; }
    for (int i = pos; i < flightCount - 1; i++) flight[i] = flight[i+1];
    flightCount--;
    saveFlight();
    packOk("删除成功", out, outSize);
}

static void handleUpdateFlight(const char** fields, int fieldCount, char* out, int outSize)
{
    if (fieldCount < 4) { packFail("修改航班参数不足", out, outSize); return; }
    char username[32];
    if (!requireAdminByToken(fields[1], username)) {
        packFail("无管理员权限", out, outSize); return;
    }
    int pos = findFlight((char*)fields[2]);
    if (pos == -1) { packFail("航班不存在", out, outSize); return; }
    float price = 0;
    if (!isPositiveFloat(fields[3], &price)) {
        packFail("票价输入无效", out, outSize); return;
    }
    flight[pos].price = price;
    saveFlight();
    packOk("修改成功", out, outSize);
}

void initServerData()
{
    InitializeCriticalSection(&g_dataMutex);
    EnterCriticalSection(&g_dataMutex);
    flightCount = 0; userCount = 0;
    front = 0; rear = 0;
    nextOrderNumber = 1;
    g_nextSessionNumber = 1;
    g_sessionCount = 0;
    loadUser();
    loadFlight();
    loadWaitQueue();
    initOrderNumber();
    LeaveCriticalSection(&g_dataMutex);
}

void handleClientRequest(const char* request, char* out, int outSize)
{
    EnterCriticalSection(&g_dataMutex);

    // Clear output
    out[0] = '\0';

    // Parse request fields
    char fields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN];
    int fieldCount = splitPacket(request, fields);

    if (fieldCount == 0 || fields[0][0] == '\0') {
        packFail("空请求", out, outSize);
        LeaveCriticalSection(&g_dataMutex);
        return;
    }

    const char* f[MAX_ORDER_FIELDS];
    for (int i = 0; i < fieldCount && i < MAX_ORDER_FIELDS; i++)
        f[i] = fields[i];

    if (strcmp(f[0], "REGISTER") == 0)
        handleRegister(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "LOGIN") == 0)
        handleLogin(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "LOGOUT") == 0)
        handleLogout(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "LIST_FLIGHTS") == 0)
        handleListFlights(out, outSize);
    else if (strcmp(f[0], "SEARCH_FLIGHT") == 0 || strcmp(f[0], "SEARCH_DESTINATION") == 0)
        handleSearchDestination(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "BOOK") == 0)
        handleBookTicket(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "REFUND") == 0)
        handleRefundTicket(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "CHANGE") == 0)
        handleChangeTicket(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "MY_ORDERS") == 0)
        handleUserOrders(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "SHOW_ORDERS") == 0)
        handleShowOrders(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "SHOW_WAITLIST") == 0)
        handleShowWaitQueue(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "SHOW_PASSENGERS") == 0)
        handleShowPassengers(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "ADD_FLIGHT") == 0)
        handleAddFlight(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "DELETE_FLIGHT") == 0)
        handleDeleteFlight(f, fieldCount, out, outSize);
    else if (strcmp(f[0], "UPDATE_FLIGHT") == 0)
        handleUpdateFlight(f, fieldCount, out, outSize);
    else
        packFail("未知请求类型", out, outSize);

    LeaveCriticalSection(&g_dataMutex);
}
