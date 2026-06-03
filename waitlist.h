#pragma once

#include "data.h"

int enqueueWaitPassenger(Passenger* p, char no[]);
void saveWaitQueue();
void loadWaitQueue();
int findFirstWaitingIndex(char flightNo[]);
int removeWaitingAt(int index, WaitingPassenger* removedPassenger);
void showWaitQueue();
