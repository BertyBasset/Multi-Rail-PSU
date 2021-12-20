#pragma once

#include "VDisplay.h" 

struct fault {
    int channelID;
    int tripCurrentMs;
};

class TripHistory {
    // last 7 items

    public:
        TripHistory();

        static const int MAX_ITEMS = 7;
        void addFault(int channelID, int tripCurrentMs);
        fault* getHistory();
        fault tripHistory[MAX_ITEMS];

    private:
        int count = 0;
 };

