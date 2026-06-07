#pragma once

#include "common.h"

typedef struct Passenger {
    char orderId[20];
    char name[20];
    char phone[20];
    char id[20];
    int ticketNum;
    struct Passenger* next;
} Passenger;

typedef struct WaitingPassenger {
    char flightNo[20];
    char name[20];
    char phone[20];
    char id[20];
    int ticketNum;
} WaitingPassenger;

typedef struct Flight {
    char flightNo[20];
    char start[20];
    char destination[20];
    char date[20];
    char startTime[20];
    char arriveTime[20];
    float price;
    int totalSeat;
    int remainSeat;
    struct Passenger* plist;
} Flight;

typedef struct User {
    char username[20];
    char password[20];
    int role;
} User;

#define MAX_FLIGHT 100
#define MAX_USER 20
#define MAX_WAIT 100
