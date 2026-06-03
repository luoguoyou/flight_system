#pragma once

#include "data.h"

extern Flight flight[MAX_FLIGHT];
extern User users[MAX_USER];
extern WaitingPassenger waitQueue[MAX_WAIT];
extern int front;
extern int rear;
extern int flightCount;
extern int userCount;
extern int nextOrderNumber;
