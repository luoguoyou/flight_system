#include "client.h"

#include "protocol.h"
#include "utils.h"

#include <cstdlib>
#include <cstdio>
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

static void showMenu(const Session& session)
{
    std::cout << "\n========== 航班订票客户端 ==========\n";
    std::cout << "当前用户：" << (session.loggedIn ? session.username : "未登录") << "\n";
    std::cout << "1 注册\n";
    std::cout << "2 登录\n";
    std::cout << "3 查票\n";
    std::cout << "4 订票\n";
    std::cout << "5 退票\n";
    std::cout << "6 改签\n";
    std::cout << "7 查看订单\n";
    std::cout << "0 退出\n";
    std::cout << "请选择：";
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

static bool sendAndPrint(TcpClient& client, const std::vector<std::string>& fields, std::vector<std::string>* raw = NULL)
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

static void doRegister(TcpClient& client)
{
    std::string username = inputLine("用户名：");
    std::string password = inputLine("密码：");
    sendAndPrint(client, { "REGISTER", username, password });
}

static void doLogin(TcpClient& client, Session& session)
{
    std::string username = inputLine("用户名：");
    std::string password = inputLine("密码：");
    std::vector<std::string> response;

    if (sendAndPrint(client, { "LOGIN", username, password }, &response) && response.size() >= 5)
    {
        session.loggedIn = true;
        session.role = atoi(response[2].c_str());
        session.username = response[3];
        session.token = response[4];
    }
}

static void doSearch(TcpClient& client)
{
    std::string key = inputLine("输入航班号/出发地/目的地/日期（直接回车显示全部）：");
    if (key.empty())
    {
        sendAndPrint(client, { "LIST_FLIGHTS" });
    }
    else
    {
        sendAndPrint(client, { "SEARCH_FLIGHT", key });
    }
}

static void doBook(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    std::string flightNo = inputLine("航班号：");
    std::string phone = inputLine("电话：");
    std::string id = inputLine("身份证：");
    std::string ticketNum = inputLine("订票数量：");

    sendAndPrint(client, { "BOOK", session.token, flightNo, phone, id, ticketNum });
}

static void doRefund(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    std::string orderId = inputLine("订单号：");
    sendAndPrint(client, { "REFUND", session.token, orderId });
}

static void doChange(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    std::string orderId = inputLine("订单号：");
    std::string newFlightNo = inputLine("新航班号：");
    sendAndPrint(client, { "CHANGE", session.token, orderId, newFlightNo });
}

static void doOrders(TcpClient& client, const Session& session)
{
    if (!requireLogin(session))
    {
        return;
    }

    sendAndPrint(client, { "MY_ORDERS", session.token });
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
    while (true)
    {
        showMenu(session);

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1")
        {
            doRegister(client);
        }
        else if (choice == "2")
        {
            doLogin(client, session);
        }
        else if (choice == "3")
        {
            doSearch(client);
        }
        else if (choice == "4")
        {
            doBook(client, session);
        }
        else if (choice == "5")
        {
            doRefund(client, session);
        }
        else if (choice == "6")
        {
            doChange(client, session);
        }
        else if (choice == "7")
        {
            doOrders(client, session);
        }
        else if (choice == "0")
        {
            if (session.loggedIn)
            {
                std::vector<std::string> response;
                client.request({ "LOGOUT", session.token }, response);
            }
            std::cout << "已退出客户端\n";
            return;
        }
        else
        {
            std::cout << "无效选择\n";
        }
    }
}
