#include "service.h"

#include "booking.h"
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

static bool isPositiveFloat(const std::string& text, float& value)
{
    if (text.empty())
    {
        return false;
    }

    char* endPtr = NULL;
    value = static_cast<float>(strtod(text.c_str(), &endPtr));
    return endPtr != NULL && *endPtr == '\0' && value > 0;
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

static bool requireAdmin(const std::vector<std::string>& fields, size_t tokenIndex, std::string& username)
{
    if (!getSessionUser(fields, tokenIndex, username))
    {
        return false;
    }

    int userIndex = findUser(username);
    return userIndex != -1 && users[userIndex].role == 0;
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
    char id[20];
    int ticketNum;

    while (fscanf(fp, "%19s%19s%19s%19s%19s%d", orderId, flightNo, name, phone, id, &ticketNum) != EOF)
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

static void appendFlightTableHeader(std::ostringstream& out)
{
    out << "====================================================================================================================\n";
    out << "航班号     出发地   目的地   日期         起飞       到达       票价       总座位     余票\n";
    out << "====================================================================================================================\n";
}

static std::string formatFlightRow(const Flight& f)
{
    char line[256];
    sprintf(line,
        "%-10s %-8s %-8s %-12s %-10s %-10s %-10.2f %-10d %-10d",
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

static std::string formatOrderBrief(const OrderRecord& order)
{
    char line[256];
    sprintf(line,
        "%-10s %-10s %-10s %-15s %-20s %-6d",
        order.orderId.c_str(),
        order.flightNo.c_str(),
        order.name.c_str(),
        order.phone.c_str(),
        order.id.c_str(),
        order.ticketNum);
    return line;
}

static std::string formatOrderDetail(const OrderRecord& order)
{
    std::ostringstream out;
    int pos = findFlight((char*)order.flightNo.c_str());

    out << "订单号：" << order.orderId
        << " 航班号：" << order.flightNo
        << " 姓名：" << order.name
        << " 电话：" << order.phone
        << " 身份证：" << order.id
        << " 票数：" << order.ticketNum;

    if (pos != -1)
    {
        out << " 航线：" << flight[pos].start << "->" << flight[pos].destination
            << " 日期：" << flight[pos].date
            << " 起飞：" << flight[pos].startTime
            << " 总金额：" << flight[pos].price * order.ticketNum;
    }

    return out.str();
}

static void confirmWaitingPassenger(int flightPos, const WaitingPassenger& waitingPassenger)
{
    Passenger passenger;
    generateOrderId(passenger.orderId);
    copyText(passenger.name, sizeof(passenger.name), waitingPassenger.name);
    copyText(passenger.phone, sizeof(passenger.phone), waitingPassenger.phone);
    copyText(passenger.id, sizeof(passenger.id), waitingPassenger.id);
    passenger.ticketNum = waitingPassenger.ticketNum;
    passenger.next = NULL;

    flight[flightPos].remainSeat -= passenger.ticketNum;
    savePassengerToFile(&passenger, flight[flightPos].flightNo);
}

static void processWaitingListForFlight(int flightPos)
{
    while (true)
    {
        int waitIndex = findFirstWaitingIndex(flight[flightPos].flightNo);
        if (waitIndex == -1)
        {
            break;
        }

        if (flight[flightPos].remainSeat < waitQueue[waitIndex].ticketNum)
        {
            break;
        }

        WaitingPassenger waitingPassenger;
        if (!removeWaitingAt(waitIndex, &waitingPassenger))
        {
            break;
        }

        confirmWaitingPassenger(flightPos, waitingPassenger);
    }

    saveWaitQueue();
}

static std::string handleLogout(const std::vector<std::string>& fields)
{
    if (fields.size() >= 2)
    {
        sessions.erase(fields[1]);
    }

    return ok("已退出登录");
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
    if (flightCount == 0)
    {
        return ok("暂无航班数据");
    }

    std::ostringstream out;
    appendFlightTableHeader(out);

    for (int i = 0; i < flightCount; i++)
    {
        out << formatFlightRow(flight[i]) << "\n";
    }

    out << "====================================================================================================================";
    return ok(out.str());
}

static std::string handleSearchDestination(const std::vector<std::string>& fields)
{
    if (fields.size() < 2 || fields[1].empty())
    {
        return fail("请输入目的地");
    }

    std::ostringstream out;
    int matched = 0;
    appendFlightTableHeader(out);

    for (int i = 0; i < flightCount; i++)
    {
        if (std::string(flight[i].destination).find(fields[1]) != std::string::npos)
        {
            out << formatFlightRow(flight[i]) << "\n";
            matched++;
        }
    }

    if (matched == 0)
    {
        return ok("未查询到符合条件的航班");
    }

    out << "====================================================================================================================";
    return ok(out.str());
}

static std::string handleBookTicket(const std::vector<std::string>& fields)
{
    if (fields.size() < 7)
    {
        return fail("订票参数不足");
    }

    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    int pos = findFlight((char*)fields[2].c_str());
    if (pos == -1)
    {
        return fail("航班不存在");
    }

    int ticketNum = 0;
    if (!isPositiveInt(fields[6], ticketNum))
    {
        return fail("订票数量必须为正整数");
    }

    if (fields[3].empty() || fields[4].empty() || fields[5].empty())
    {
        return fail("姓名、电话、身份证不能为空");
    }

    if (flight[pos].remainSeat < ticketNum)
    {
        if (fields.size() >= 8 && fields[7] == "WAIT")
        {
            Passenger passenger;
            copyText(passenger.name, sizeof(passenger.name), fields[3]);
            copyText(passenger.phone, sizeof(passenger.phone), fields[4]);
            copyText(passenger.id, sizeof(passenger.id), fields[5]);
            passenger.ticketNum = ticketNum;
            passenger.next = NULL;

            if (!enqueueWaitPassenger(&passenger, flight[pos].flightNo))
            {
                return fail("候补队列已满，加入失败");
            }

            saveWaitQueue();
            return ok("余票不足，已加入候补队列");
        }

        return fail("余票不足，当前余票：" + std::to_string(flight[pos].remainSeat));
    }

    Passenger passenger;
    generateOrderId(passenger.orderId);
    copyText(passenger.name, sizeof(passenger.name), fields[3]);
    copyText(passenger.phone, sizeof(passenger.phone), fields[4]);
    copyText(passenger.id, sizeof(passenger.id), fields[5]);
    passenger.ticketNum = ticketNum;
    passenger.next = NULL;

    flight[pos].remainSeat -= ticketNum;
    savePassengerToFile(&passenger, flight[pos].flightNo);
    saveFlight();

    std::ostringstream out;
    out << "====================================\n";
    out << "               订票成功\n";
    out << "====================================\n";
    out << "订单编号：" << passenger.orderId << "\n";
    out << "航班号：" << flight[pos].flightNo << "\n";
    out << "航线：" << flight[pos].start << " -> " << flight[pos].destination << "\n";
    out << "日期：" << flight[pos].date << "\n";
    out << "起飞时间：" << flight[pos].startTime << "\n";
    out << "乘客姓名：" << passenger.name << "\n";
    out << "联系电话：" << passenger.phone << "\n";
    out << "身份证号：" << passenger.id << "\n";
    out << "票价：" << flight[pos].price << " 元\n";
    out << "购买数量：" << passenger.ticketNum << " 张\n";
    out << "总金额：" << flight[pos].price * passenger.ticketNum << " 元\n";
    out << "剩余票数：" << flight[pos].remainSeat << " 张\n";
    out << "====================================";
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
        if (it->orderId == fields[2])
        {
            removed = *it;
            orders.erase(it);
            found = true;
            break;
        }
    }

    if (!found)
    {
        return fail("未找到该订单号");
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
    if (pos != -1)
    {
        processWaitingListForFlight(pos);
    }
    saveFlight();

    std::ostringstream out;
    out << "====================================\n";
    out << "                退票成功\n";
    out << "====================================\n";
    out << "订单编号：" << removed.orderId << "\n";
    out << "乘客姓名：" << removed.name << "\n";

    if (pos != -1)
    {
        out << "航班号：" << removed.flightNo << "\n";
        out << "航线：" << flight[pos].start << " -> " << flight[pos].destination << "\n";
        out << "退票数量：" << removed.ticketNum << " 张\n";
        out << "当前余票：" << flight[pos].remainSeat << " 张\n";
    }

    out << "====================================";
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
        if (orders[i].orderId == fields[2])
        {
            orderIndex = (int)i;
            break;
        }
    }

    if (orderIndex == -1)
    {
        return fail("未找到该订单号");
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
        processWaitingListForFlight(oldFlight);
    }

    flight[newFlight].remainSeat -= orders[orderIndex].ticketNum;
    orders[orderIndex].flightNo = fields[3];

    saveOrders(orders);
    saveFlight();

    std::ostringstream out;
    out << "改签成功\n" << formatOrderDetail(orders[orderIndex]);
    return ok(out.str());
}

static std::string handleShowOrders(const std::vector<std::string>& fields)
{
    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    std::vector<OrderRecord> orders = loadOrders();
    if (orders.empty())
    {
        return ok("暂无订单记录");
    }

    std::ostringstream out;
    out << "================================================================================\n";
    out << "订单号     航班号     姓名       电话            身份证               票数\n";
    out << "================================================================================\n";

    for (const OrderRecord& order : orders)
    {
        out << formatOrderBrief(order) << "\n";
    }

    out << "================================================================================";
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
            out << formatOrderDetail(order) << "\n";
            matched++;
        }
    }

    if (matched == 0)
    {
        return ok("暂无订单");
    }

    return ok(out.str());
}

static std::string handleShowWaitQueue(const std::vector<std::string>& fields)
{
    std::string username;
    if (!getSessionUser(fields, 1, username))
    {
        return fail("登录状态无效，请重新登录");
    }

    if (front == rear)
    {
        return ok("当前暂无候补乘客");
    }

    std::ostringstream out;
    out << "================================================================================\n";
    out << "序号   航班号     姓名       电话            身份证               票数\n";
    out << "================================================================================\n";

    for (int i = front; i < rear; i++)
    {
        char line[256];
        sprintf(line,
            "%-6d %-10s %-10s %-15s %-20s %-6d",
            i - front + 1,
            waitQueue[i].flightNo,
            waitQueue[i].name,
            waitQueue[i].phone,
            waitQueue[i].id,
            waitQueue[i].ticketNum);
        out << line << "\n";
    }

    out << "================================================================================";
    return ok(out.str());
}

static std::string handleShowPassengers(const std::vector<std::string>& fields)
{
    std::string username;
    if (!requireAdmin(fields, 1, username))
    {
        return fail("无管理员权限");
    }

    std::vector<OrderRecord> orders = loadOrders();
    std::ostringstream out;
    out << "====================== 航班及订票客户信息 ======================\n";

    for (int i = 0; i < flightCount; i++)
    {
        bool hasPassenger = false;
        out << "\n==========================================================\n";
        out << "航班号：" << flight[i].flightNo << "  日期：" << flight[i].date << "\n";
        out << "出发地：" << flight[i].start << "  目的地：" << flight[i].destination << "\n";
        out << "起飞时间：" << flight[i].startTime << "  到达时间：" << flight[i].arriveTime << "\n";
        out << "总座位：" << flight[i].totalSeat << "  剩余票数：" << flight[i].remainSeat << "\n";
        out << "----------------------------------------------------------\n";

        for (const OrderRecord& order : orders)
        {
            if (order.flightNo == flight[i].flightNo)
            {
                if (!hasPassenger)
                {
                    out << "订单号       姓名       电话            身份证               票数\n";
                    out << "==========================================================\n";
                    hasPassenger = true;
                }

                char line[256];
                sprintf(line,
                    "%-12s %-10s %-15s %-20s %-8d",
                    order.orderId.c_str(),
                    order.name.c_str(),
                    order.phone.c_str(),
                    order.id.c_str(),
                    order.ticketNum);
                out << line << "\n";
            }
        }

        if (!hasPassenger)
        {
            out << "暂无订票客户\n";
        }
    }

    out << "\n====================== 信息显示完毕 ======================";
    return ok(out.str());
}

static std::string handleAddFlight(const std::vector<std::string>& fields)
{
    if (fields.size() < 10)
    {
        return fail("新增航班参数不足");
    }

    std::string username;
    if (!requireAdmin(fields, 1, username))
    {
        return fail("无管理员权限");
    }

    if (flightCount >= MAX_FLIGHT)
    {
        return fail("航班数量已满");
    }

    if (findFlight((char*)fields[2].c_str()) != -1)
    {
        return fail("航班号已存在");
    }

    float price = 0;
    int totalSeat = 0;
    if (!isPositiveFloat(fields[8], price) || !isPositiveInt(fields[9], totalSeat))
    {
        return fail("票价或总座位输入无效");
    }

    Flight newFlight;
    copyText(newFlight.flightNo, sizeof(newFlight.flightNo), fields[2]);
    copyText(newFlight.start, sizeof(newFlight.start), fields[3]);
    copyText(newFlight.destination, sizeof(newFlight.destination), fields[4]);
    copyText(newFlight.date, sizeof(newFlight.date), fields[5]);
    copyText(newFlight.startTime, sizeof(newFlight.startTime), fields[6]);
    copyText(newFlight.arriveTime, sizeof(newFlight.arriveTime), fields[7]);
    newFlight.price = price;
    newFlight.totalSeat = totalSeat;
    newFlight.remainSeat = totalSeat;
    newFlight.plist = NULL;

    flight[flightCount++] = newFlight;
    saveFlight();
    return ok("新增成功");
}

static std::string handleDeleteFlight(const std::vector<std::string>& fields)
{
    if (fields.size() < 3)
    {
        return fail("删除航班参数不足");
    }

    std::string username;
    if (!requireAdmin(fields, 1, username))
    {
        return fail("无管理员权限");
    }

    int pos = findFlight((char*)fields[2].c_str());
    if (pos == -1)
    {
        return fail("航班不存在");
    }

    for (int i = pos; i < flightCount - 1; i++)
    {
        flight[i] = flight[i + 1];
    }

    flightCount--;
    saveFlight();
    return ok("删除成功");
}

static std::string handleUpdateFlight(const std::vector<std::string>& fields)
{
    if (fields.size() < 4)
    {
        return fail("修改航班参数不足");
    }

    std::string username;
    if (!requireAdmin(fields, 1, username))
    {
        return fail("无管理员权限");
    }

    int pos = findFlight((char*)fields[2].c_str());
    if (pos == -1)
    {
        return fail("航班不存在");
    }

    float price = 0;
    if (!isPositiveFloat(fields[3], price))
    {
        return fail("票价输入无效");
    }

    flight[pos].price = price;
    saveFlight();
    return ok("修改成功");
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
    if (fields[0] == "SEARCH_FLIGHT" || fields[0] == "SEARCH_DESTINATION")
    {
        return handleSearchDestination(fields);
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
    if (fields[0] == "SHOW_ORDERS")
    {
        return handleShowOrders(fields);
    }
    if (fields[0] == "SHOW_WAITLIST")
    {
        return handleShowWaitQueue(fields);
    }
    if (fields[0] == "SHOW_PASSENGERS")
    {
        return handleShowPassengers(fields);
    }
    if (fields[0] == "ADD_FLIGHT")
    {
        return handleAddFlight(fields);
    }
    if (fields[0] == "DELETE_FLIGHT")
    {
        return handleDeleteFlight(fields);
    }
    if (fields[0] == "UPDATE_FLIGHT")
    {
        return handleUpdateFlight(fields);
    }

    return fail("未知请求类型：" + fields[0]);
}
