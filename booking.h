#pragma once

#include "data.h"

void savePassengerToFile(Passenger* p, char flightNo[]);
void deletePassengerFromFile(char orderId[]);
void bookTicket();
void refundTicket();
