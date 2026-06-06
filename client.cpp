#include "client.h"

#include "protocol.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class TcpClient
{
public:
    TcpClient() : sock(INVALID_SOCKET), wsaReady(false)
    {
    }

    ~TcpClient()
    {
        close();
        if (wsaReady)
        {
            WSACleanup();
        }
    }

    bool init()
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cout << "WinSock 初始化失败\n";
            return false;
        }

        wsaReady = true;
        return connectServer();
    }

    bool request(const std::vector<std::string>& fields, std::vector<std::string>& response)
    {
        std::string packet = makePacket(fields);

        for (int attempt = 0; attempt < 2; attempt++)
        {
            if (!isConnected() && !connectServer())
            {
                continue;
            }

            if (sendLine(packet) && readLine(packet))
            {
                response = splitPacket(packet);
                return true;
            }

            std::cout << "连接异常，正在尝试重连...\n";
            close();
        }

        return false;
    }

private:
    SOCKET sock;
    bool wsaReady;

    bool isConnected() const
    {
        return sock != INVALID_SOCKET;
    }

    void close()
    {
        if (sock != INVALID_SOCKET)
        {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
    }

    bool connectServer()
    {
        close();

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
        {
            std::cout << "创建 socket 失败\n";
            return false;
        }

        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8888);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            close();
            std::cout << "无法连接服务器 127.0.0.1:8888，请先启动服务端\n";
            return false;
        }

        std::cout << "已连接服务器 127.0.0.1:8888\n";
        return true;
    }

    bool sendLine(const std::string& packet)
    {
        std::string text = packet + "\n";
        const char* data = text.c_str();
        int left = (int)text.size();

        while (left > 0)
        {
            int sent = send(sock, data, left, 0);
            if (sent <= 0)
            {
                return false;
            }

            data += sent;
            left -= sent;
        }

        return true;
    }

    bool readLine(std::string& line)
    {
        line.clear();
        char ch = '\0';

        while (true)
        {
            int received = recv(sock, &ch, 1, 0);
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
};

struct Session
{
    bool loggedIn = false;
    std::string username;
    std::string token;
    int role = 1;
};

static std::string inputLine(const std::string& prompt)
{
    std::string value;
    std::cout << prompt;
    std::getline(std::cin, value);
    return value;
}

static bool sendRequest(TcpClient& client, const std::vector<std::string>& fields, std::vector<std::string>* raw = NULL)
{
    std::vector<std::string> response;

    if (!client.request(fields, response))
    {
        std::cout << "请求失败，服务器不可用\n";
        return false;
    }

    if (raw != NULL)
    {
        *raw = response;
    }

    if (response.size() >= 2)
    {
        std::cout << "\n" << response[1] << "\n";
    }
    else
    {
        std::cout << "\n服务器返回格式异常\n";
    }

    return !response.empty() && response[0] == "OK";
}

static bool requireLogin(const Session& session)
{
    if (!session.loggedIn)
    {
        std::cout << "请先登录\n";
        return false;
    }

    return true;
}

static bool requireAdmin(const Session& session)
{
    if (!requireLogin(session))
    {
        return false;
    }

    if (session.role != 0)
    {
        std::cout << "当前账号不是管理员\n";
        return false;
    }

    return true;
}

static bool loginFlow(TcpClient& client, Session& session)
{
    while (true)
    {
        std::cout << "\n1 注册\n";
        std::cout << "2 登录\n";
        std::cout << "0 退出系统\n";
        std::cout << "请选择：";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1")
        {
            std::string username = inputLine("用户名:");
            std::string password = inputLine("密码:");
            sendRequest(client, { "REGISTER", username, password });
            continue;
        }

        if (choice == "2")
        {
            std::string username = inputLine("用户名:");
            std::string password = inputLine("密码:");
            std::vector<std::string> response;

            if (sendRequest(client, { "LOGIN", username, password }, &response) && response.size() >= 5)
            {
                session.loggedIn = true;
                session.role = atoi(response[2].c_str());
                session.username = response[3];
                session.token = response[4];
                return true;
            }

            continue;
        }

        if (choice == "0")
        {
            return false;
        }

        std::cout << "无效选择\n";
    }
}

static void userMenu()
{
    printf("\n");
    printf("========== 用户菜单 ==========\n");
    printf("1 显示航班\n");
    printf("2 查询航班\n");
    printf("3 办理订票\n");
    printf("4 办理退票\n");
    printf("5 查看订单记录\n");
    printf("6 查看候补队列\n");
    printf("0 退出系统\n");
}

static void adminMenu()
{
    printf("\n");
    printf("========== 管理员菜单 ==========\n");
    printf("1 显示航班\n");
    printf("2 查询航班\n");
    printf("3 新增航班\n");
    printf("4 删除航班\n");
    printf("5 修改航班\n");
    printf("6 查看订票客户\n");
    printf("7 查看订单记录\n");
    printf("8 查看候补队列\n");
    printf("0 退出系统\n");
}

static void showFlights(TcpClient& client)
{
    sendRequest(client, { "LIST_FLIGHTS" });
}

static void searchFlights(TcpClient& client)
{
    std::string destination = inputLine("请输入目的地:");
    sendRequest(client, { "SEARCH_DESTINATION", destination });
}

static void bookTicket(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    std::string flightNo = inputLine("请输入航班号:");
    std::string name = inputLine("请输入姓名:");
    std::string phone = inputLine("请输入电话:");
    std::string id = inputLine("请输入身份证号:");
    std::string ticketNum = inputLine("请输入订票数量:");

    std::vector<std::string> response;
    if (sendRequest(client, { "BOOK", session.token, flightNo, name, phone, id, ticketNum }, &response))
    {
        return;
    }

    if (response.size() >= 2 && response[1].find("余票不足") != std::string::npos)
    {
        std::string choice = inputLine("是否进入候补队列？1-是 0-否:");
        if (choice == "1")
        {
            sendRequest(client, { "BOOK", session.token, flightNo, name, phone, id, ticketNum, "WAIT" });
        }
    }
}

static void refundTicket(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    std::string orderId = inputLine("请输入订单号:");
    sendRequest(client, { "REFUND", session.token, orderId });
}

static void showOrderFile(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    sendRequest(client, { "SHOW_ORDERS", session.token });
}

static void showWaitQueue(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    sendRequest(client, { "SHOW_WAITLIST", session.token });
}

static void addFlight(TcpClient& client, const Session& session)
{
    if (!requireAdmin(session))
    {
        return;
    }

    std::string flightNo = inputLine("航班号:");
    std::string start = inputLine("出发地:");
    std::string destination = inputLine("目的地:");
    std::string date = inputLine("日期:");
    std::string startTime = inputLine("起飞时间:");
    std::string arriveTime = inputLine("到达时间:");
    std::string price = inputLine("票价:");
    std::string totalSeat = inputLine("总座位:");

    sendRequest(client, { "ADD_FLIGHT", session.token, flightNo, start, destination, date, startTime, arriveTime, price, totalSeat });
}

static void deleteFlight(TcpClient& client, const Session& session)
{
    if (!requireAdmin(session))
    {
        return;
    }

    std::string flightNo = inputLine("输入航班号:");
    sendRequest(client, { "DELETE_FLIGHT", session.token, flightNo });
}

static void updateFlight(TcpClient& client, const Session& session)
{
    if (!requireAdmin(session))
    {
        return;
    }

    std::string flightNo = inputLine("请输入航班号:");
    std::string price = inputLine("输入新票价:");
    sendRequest(client, { "UPDATE_FLIGHT", session.token, flightNo, price });
}

static void showPassengers(TcpClient& client, const Session& session)
{
    if (!requireAdmin(session))
    {
        return;
    }

    sendRequest(client, { "SHOW_PASSENGERS", session.token });
}

void runClient()
{
    initConsole();

    TcpClient client;
    if (!client.init())
    {
        return;
    }

    Session session;
    if (!loginFlow(client, session))
    {
        return;
    }

    while (true)
    {
        std::string choice;

        if (session.role == 0)
        {
            adminMenu();
            std::getline(std::cin, choice);

            if (choice == "1")
            {
                showFlights(client);
            }
            else if (choice == "2")
            {
                searchFlights(client);
            }
            else if (choice == "3")
            {
                addFlight(client, session);
            }
            else if (choice == "4")
            {
                deleteFlight(client, session);
            }
            else if (choice == "5")
            {
                updateFlight(client, session);
            }
            else if (choice == "6")
            {
                showPassengers(client, session);
            }
            else if (choice == "7")
            {
                showOrderFile(client, session);
            }
            else if (choice == "8")
            {
                showWaitQueue(client, session);
            }
            else if (choice == "0")
            {
                std::vector<std::string> response;
                client.request({ "LOGOUT", session.token }, response);
                return;
            }
            else
            {
                std::cout << "无效选择\n";
            }

            continue;
        }

        userMenu();
        std::getline(std::cin, choice);

        if (choice == "1")
        {
            showFlights(client);
        }
        else if (choice == "2")
        {
            searchFlights(client);
        }
        else if (choice == "3")
        {
            bookTicket(client, session);
        }
        else if (choice == "4")
        {
            refundTicket(client, session);
        }
        else if (choice == "5")
        {
            showOrderFile(client, session);
        }
        else if (choice == "6")
        {
            showWaitQueue(client, session);
        }
        else if (choice == "0")
        {
            std::vector<std::string> response;
            client.request({ "LOGOUT", session.token }, response);
            return;
        }
        else
        {
            std::cout << "无效选择\n";
        }
    }
}
