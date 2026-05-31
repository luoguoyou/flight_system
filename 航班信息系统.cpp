#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "flight.h"
#include "passenger.h"
#include "user.h"
using namespace std;
//主函数
int main()
{
    int role;
    int choice;
    loadUser();
    loadFlight();
    role = login();
    if (role == -1)
    {
        printf("登录失败\n");
        return 0;
    }
    if (role == 0)
    {
        while (1)
        {
            adminMenu();
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                showFlight();
                break;

            case 2:
                searchDestination();
                break;

            case 3:
                addFlight();
                break;

            case 4:
                deleteFlight();
                break;

            case 5:
                updateFlight();
                break;

            case 6:
                showPassenger();
                break;

            case 7:
                showOrderFile();
                break;

            case 0:
                return 0;
            }
        }
    }
    else
    {
        while (1)
        {
            userMenu();

            scanf("%d", &choice);

            switch (choice)
            {
            case 1:
                showFlight();
                break;

            case 2:
                searchDestination();
                break;

            case 3:
                bookTicket();
                break;

            case 4:
                refundTicket();
                break;

            case 5:
                showOrderFile();
                break;

            case 0:
                return 0;
            }
        }
    }
    
   
}