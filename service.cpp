#include "service.h"

#include "flight_ops.h"
#include "globals.h"
#include "protocol.h"
#include "user.h"
#include "utils.h"
#include "waitlist.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static std::mutex dataMutex;
static std::unordered_map<std::string, std::string> sessions;
static int nextSessionNumber = 1;

struct OrderRecord
{
    std::string orderId;
    std::string flightNo;
    std::string name;
    std::string phone;
    std::string id;
    int ticketNum;
};

static std::string ok(const std::string& body)
{
    return makePacket({ "OK", body });
}

static std::string fail(const std::string& body)
{
    return makePacket({ "ERR", body });
}

static void copyText(char target[], size_t size, const std::string& value)
{
    if (size == 0)
    {
        return;
    }

    strncpy(target, value.c_str(), size - 1);
    target[size - 1] = '\0';
}

static bool isPositiveInt(const std::string& text, int& value)
{
    if (text.empty())
    {
        return false;
    }

    for (char ch : text)
    {
        if (ch < '0' || ch > '9')
        {
            return false;
        }
    }

    value = atoi(text.c_str());
    return value > 0;
}

static int findUser(const std::string& username)
{
    for (int i = 0; i < userCount; i++)
    {
        if (username == users[i].username)
        {
            return i;
        }
    }

    return -1;
}

static std::string createSession(const std::string& username)
{
    std::string token = "S" + std::to_string(nextSessionNumber++);
    sessions[token] = username;
    return token;
}

static bool getSessionUser(const std::vector<std::string>& fields, size_t tokenIndex, std::string& username)
{
    if (fields.size() <= tokenIndex)
    {
        return false;
    }

    auto it = sessions.find(fields[tokenIndex]);
    if (it == sessions.end())
    {
        return false;
    }

    username = it->second;
    return true;
}

static std::string handleLogout(const std::vector<std::string>& fields)
{
    if (fields.size() >= 2)
    {
        sessions.erase(fields[1]);
    }

    return ok("已退出登录");
}

static void saveUsers()
{
    FILE* fp = fopen("user.txt", "w");

    if (fp == NULL)
    {
        return;
    }

    for (int i = 0; i < userCount; i++)
    {
        fprintf(fp, "%s %s %d\n", users[i].username, users[i].password, users[i].role);
    }

    fclose(fp);
}

static std::vector<OrderRecord> loadOrders()
{
    std::vector<OrderRecord> orders;
    FILE* fp = fopen("passenger.txt", "r");

    if (fp == NULL)
    {
        return orders;
    }

    OrderRecord order;
    char orderId[20];
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[30];
    int ticketNum;

    while (fscanf(fp, "%19s%19s%19s%19s%29s%d", orderId, flightNo, name, phone, id, &ticketNum) != EOF)
    {
        stripBom(orderId);
        order.orderId = orderId;
        order.flightNo = flightNo;
        order.name = name;
        order.phone = phone;
        order.id = id;
        order.ticketNum = ticketNum;
        orders.push_back(order);
    }

    fclose(fp);
    return orders;
}

static bool saveOrders(const std::vector<OrderRecord>& orders)
{
    FILE* fp = fopen("passenger.txt", "w");

    if (fp == NULL)
    {
        return false;
    }

    for (const OrderRecord& order : orders)
    {
        fprintf(fp,
            "%s %s %s %s %s %d\n",
            order.orderId.c_str(),
            order.flightNo.c_str(),
            order.name.c_str(),
            order.phone.c_str(),
            order.id.c_str(),
            order.ticketNum);
    }

    fclose(fp);
    return true;
}

static std::string formatFlight(const Flight& f)
{
    char line[256];
    sprintf(line,
        "%-10s %-10s -> %-10s 日期:%-12s 起飞:%-8s 到达:%-8s 票价:%.2f 总座位:%d 余票:%d",
        f.flightNo,
        f.start,
        f.destination,
        f.date,
        f.startTime,
        f.arriveTime,
        f.price,
        f.totalSeat,
        f.remainSeat);
    return line;
}

static std::string formatOrder(const OrderRecord& order)
{
    std::ostringstream out;
    int pos = findFlight((char*)order.flightNo.c_str());

    out << "订单号:" << order.orderId
        << " 航班:" << order.flightNo
        << " 姓名:" << order.name
        << " 电话:" << order.phone
        << " 身份证:" << order.id
        << " 票数:" << order.ticketNum;

    if (pos != -1)
    {
        out << " 航线:" << flight[pos].start << "->" << flight[pos].destination
            << " 日期:" << flight[pos].date
            << " 起飞:" << flight[pos].startTime
            << " 总价:" << flight[pos].price * order.ticketNum;
    }

    return out.str();
}

static std::string handleRegister(const std::vector<std::string>& fields)
{
    if (fields.size() < 3)
    {
        return fail("注册参数不足");
    }

    if (fields[1].empty() || fields[2].empty())
    {
        return fail("用户名和密码不能为空");
    }

    if (findUser(fields[1]) != -1)
    {
        return fail("用户名已存在，请更换用户名");
    }

    if (userCount >= MAX_USER)
    {
        return fail("用户容量已满，无法注册");
    }

    copyText(users[userCount].username, sizeof(users[userCount].username), fields[1]);
    copyText(users[userCount].password, sizeof(users[userCount].password), fields[2]);
    users[userCount].role = 1;
    userCount++;
    saveUsers();

    return ok("注册成功，请登录");
}

static std::string handleLogin(const std::vector<std::string>& fields)
{
    if (fields.size() < 3)
    {
        return fail("登录参数不足");
    }

    int index = findUser(fields[1]);
    if (index == -1 || fields[2] != users[index].password)
    {
        return fail("账号或密码错误");
    }

    std::string token = createSession(fields[1]);
    return makePacket({ "OK", "登录成功", std::to_string(users[index].role), fields[1], token });
}

static std::string handleListFlights()
{
    std::ostringstream out;

    if (flightCount == 0)
    {
        return ok("暂无航班数据");
    }

    for (int i = 0; i < flightCount; i++)
    {
        out << formatFlight(flight[i]) << "\n";
    }

    return ok(out.str());
}

static std::string handleSearchFlight(const std::vector<std::string>& fields)
{
    std::string key = fields.size() >= 2 ? fields[1] : "";
    std::ostringstream out;
    int matched = 0;

    for (int i = 0; i < flightCount; i++)
    {
        if (key.empty() ||
            std::string(flight[i].flightNo).find(key) != std::string::npos ||
            std::string(flight[i].start).find(key) != std::string::npos ||
            std::string(flight[i].destination).find(key) != std::string::npos ||
            std::string(flight[i].date).find(key) != std::string::npos)
        {
            out << formatFlight(flight[i]) << "\n";
            matched++;
        }
    }

    if (matched == 0)
    {
        return ok("未查询到符合条件的航班");
    }

    return ok(out.str());
}

static std::string handleBookTicket(const std::vector<std::string>& fields)
{
    if (fields.size() < 6)
    {
        return fail("订票参数不足");
    }

    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    int ticketNum = 0;
    if (!isPositiveInt(fields[5], ticketNum))
    {
        return fail("订票数量必须为正整数");
    }

    int pos = findFlight((char*)fields[2].c_str());
    if (pos == -1)
    {
        return fail("航班不存在");
    }

    if (flight[pos].remainSeat < ticketNum)
    {
        return fail("余票不足，当前余票：" + std::to_string(flight[pos].remainSeat));
    }

    Passenger p;
    generateOrderId(p.orderId);
    copyText(p.name, sizeof(p.name), username);
    copyText(p.phone, sizeof(p.phone), fields[3]);
    copyText(p.id, sizeof(p.id), fields[4]);
    p.ticketNum = ticketNum;
    p.next = NULL;

    flight[pos].remainSeat -= ticketNum;
    savePassengerToFile(&p, flight[pos].flightNo);
    saveFlight();

    std::ostringstream out;
    out << "订票成功\n"
        << "订单号:" << p.orderId << "\n"
        << "用户:" << username << "\n"
        << "航班:" << flight[pos].flightNo << "\n"
        << "航线:" << flight[pos].start << "->" << flight[pos].destination << "\n"
        << "日期:" << flight[pos].date << "\n"
        << "起飞:" << flight[pos].startTime << "\n"
        << "票数:" << ticketNum << "\n"
        << "总价:" << flight[pos].price * ticketNum << "\n"
        << "剩余票数:" << flight[pos].remainSeat;

    return ok(out.str());
}

static std::string handleRefundTicket(const std::vector<std::string>& fields)
{
    if (fields.size() < 3)
    {
        return fail("退票参数不足");
    }

    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    std::vector<OrderRecord> orders = loadOrders();
    bool found = false;
    OrderRecord removed;

    for (auto it = orders.begin(); it != orders.end(); ++it)
    {
        if (it->orderId == fields[2] && it->name == username)
        {
            removed = *it;
            orders.erase(it);
            found = true;
            break;
        }
    }

    if (!found)
    {
        return fail("未找到该用户的订单");
    }

    int pos = findFlight((char*)removed.flightNo.c_str());
    if (pos != -1)
    {
        flight[pos].remainSeat += removed.ticketNum;
        if (flight[pos].remainSeat > flight[pos].totalSeat)
        {
            flight[pos].remainSeat = flight[pos].totalSeat;
        }
    }

    saveOrders(orders);
    saveFlight();

    std::ostringstream out;
    out << "退票成功\n"
        << formatOrder(removed);
    return ok(out.str());
}

static std::string handleChangeTicket(const std::vector<std::string>& fields)
{
    if (fields.size() < 4)
    {
        return fail("改签参数不足");
    }

    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    std::vector<OrderRecord> orders = loadOrders();
    int orderIndex = -1;

    for (size_t i = 0; i < orders.size(); i++)
    {
        if (orders[i].orderId == fields[2] && orders[i].name == username)
        {
            orderIndex = (int)i;
            break;
        }
    }

    if (orderIndex == -1)
    {
        return fail("未找到该用户的订单");
    }

    int oldFlight = findFlight((char*)orders[orderIndex].flightNo.c_str());
    int newFlight = findFlight((char*)fields[3].c_str());

    if (newFlight == -1)
    {
        return fail("目标航班不存在");
    }

    if (orders[orderIndex].flightNo == fields[3])
    {
        return fail("目标航班与原航班相同");
    }

    if (flight[newFlight].remainSeat < orders[orderIndex].ticketNum)
    {
        return fail("目标航班余票不足，当前余票：" + std::to_string(flight[newFlight].remainSeat));
    }

    if (oldFlight != -1)
    {
        flight[oldFlight].remainSeat += orders[orderIndex].ticketNum;
        if (flight[oldFlight].remainSeat > flight[oldFlight].totalSeat)
        {
            flight[oldFlight].remainSeat = flight[oldFlight].totalSeat;
        }
    }

    flight[newFlight].remainSeat -= orders[orderIndex].ticketNum;
    orders[orderIndex].flightNo = fields[3];

    saveOrders(orders);
    saveFlight();

    std::ostringstream out;
    out << "改签成功\n"
        << formatOrder(orders[orderIndex]);
    return ok(out.str());
}

static std::string handleUserOrders(const std::vector<std::string>& fields)
{
    if (fields.size() < 2)
    {
        return fail("查询订单参数不足");
    }

    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    std::vector<OrderRecord> orders = loadOrders();
    std::ostringstream out;
    int matched = 0;

    for (const OrderRecord& order : orders)
    {
        if (order.name == username)
        {
            out << formatOrder(order) << "\n";
            matched++;
        }
    }

    if (matched == 0)
    {
        return ok("暂无订单");
    }

    return ok(out.str());
}

void initServerData()
{
    std::lock_guard<std::mutex> guard(dataMutex);

    flightCount = 0;
    userCount = 0;
    front = 0;
    rear = 0;
    nextOrderNumber = 1;
    nextSessionNumber = 1;
    sessions.clear();

    loadUser();
    loadFlight();
    loadWaitQueue();
    initOrderNumber();
}

std::string handleClientRequest(const std::string& request)
{
    std::lock_guard<std::mutex> guard(dataMutex);
    std::vector<std::string> fields = splitPacket(request);

    if (fields.empty() || fields[0].empty())
    {
        return fail("空请求");
    }

    if (fields[0] == "REGISTER")
    {
        return handleRegister(fields);
    }
    if (fields[0] == "LOGIN")
    {
        return handleLogin(fields);
    }
    if (fields[0] == "LOGOUT")
    {
        return handleLogout(fields);
    }
    if (fields[0] == "LIST_FLIGHTS")
    {
        return handleListFlights();
    }
    if (fields[0] == "SEARCH_FLIGHT")
    {
        return handleSearchFlight(fields);
    }
    if (fields[0] == "BOOK")
    {
        return handleBookTicket(fields);
    }
    if (fields[0] == "REFUND")
    {
        return handleRefundTicket(fields);
    }
    if (fields[0] == "CHANGE")
    {
        return handleChangeTicket(fields);
    }
    if (fields[0] == "MY_ORDERS")
    {
        return handleUserOrders(fields);
    }

    return fail("未知请求类型：" + fields[0]);
}
