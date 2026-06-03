#pragma once

#include "common.h"

struct Passenger
{
    char orderId[20];
    char name[20];
    char phone[20];
    char id[20];
    int ticketNum;
    Passenger* next;
};

struct WaitingPassenger
{
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[20];
    int ticketNum;
};

struct Flight
{
    char flightNo[20];
    char start[20];
    char destination[20];
    char date[20];
    char startTime[20];
    char arriveTime[20];
    float price;
    int totalSeat;
    int remainSeat;
    Passenger* plist;
};

struct User
{
    char username[20];
    char password[20];
    int role;
};

constexpr int MAX_FLIGHT = 100;
constexpr int MAX_USER = 20;
constexpr int MAX_WAIT = 100;
