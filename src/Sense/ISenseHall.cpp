#include "ISenseHall.h"

// Constructor
ISenseHall::ISenseHall() {}

ISenseHall::ISenseHall(ADC* adc, int analogPin, float iScale, float iOffset) {
    ISenseHall::adc = adc;
    ISenseHall::analogPin = analogPin;
    ISenseHall::m = iScale;         // For thsi treat as y = m x + c
    ISenseHall::c = iOffset;
}


// Maintain cyclic averaging buffer if there is one
void ISenseHall::update() {
    // This might need to be configured per instance if it varies across devices
    double voltage= (double)(adc->analogRead(analogPin)) * resolution;
    float mA = (voltage * m + c) * 1000.0 / 3.0;

    // Tweak -5V chan for some reason
    if(analogPin == A4)
        mA = mA * 4.0 / 3.0;

    bufferIndex++;
    if(bufferIndex >= BUFFER_SIZE)
        bufferIndex = 0;
    Buffer[bufferIndex] = mA;
}


// Return value, or average from cyclic buffer
int ISenseHall::read() {
    long sum = 0;
    for(int i = 0; i < BUFFER_SIZE; i++)
        sum+= Buffer[i];

    double mA = sum / (double)BUFFER_SIZE;

    // NOTE - If multiple -ve rails are on, we get 20mA or so appearing on channels with no load
    // Need to live with this. 

    // Quantise 10 here
    //return mA;
    mA -= 5;
    mA = (int)(mA / 10.0) * 10.0;

    if(mA < 0)
        mA = 0;

    return(mA);
}