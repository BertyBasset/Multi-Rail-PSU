#include "TripHistory.h"


TripHistory::TripHistory() {
    // Initialise array of fault structs
    for(int i = 0; i < MAX_ITEMS; i++)
        tripHistory[i] = fault{-1, -1};
}


void TripHistory::addFault(int channelID, int tripCurrentMs) {
    // add struct to LIFO structure

    //                  0   1   2   3   n
    //                  a   b   c   d   x   
    // newitem -> newItem   a   b   e   .


    for(int i = MAX_ITEMS - 1; i >= 1; i--) {
        tripHistory[i].channelID = tripHistory[i - 1].channelID;
        tripHistory[i].tripCurrentMs = tripHistory[i - 1].tripCurrentMs;
    }

    tripHistory[0].channelID = channelID;
    tripHistory[0].tripCurrentMs =tripCurrentMs;
}

 