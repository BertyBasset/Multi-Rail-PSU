#include <Arduino.h>
#include "ISenseRes.h"

// Constructor
ISenseRes::ISenseRes() {}

ISenseRes::ISenseRes(ADC* adc, int analogPin, float rSense, float scale, float offset) {
    ISenseRes::adc = adc;
    ISenseRes::analogPin = analogPin;
    ISenseRes::rSense = rSense;
    ISenseRes::iScale = scale;
    ISenseRes::iOffset = offset;
}


// Maintain cyclic averaging buffer if there is one, also return last value for tripping
void ISenseRes::update() {
    float voltage= (float)(adc->analogRead(analogPin)) * resolution;
    // Apply calibration here
    float i = ((voltage / (rSense * gain) * iScale) + iOffset) * 1000.0;   // x1000 A->mA

    bufferIndex++;
    if(bufferIndex >= BUFFER_SIZE)
        bufferIndex = 0;
    Buffer[bufferIndex] = i;
}


// Return value, or average from cyclic buffer
// Return mA - maybe quantize ?
int ISenseRes::read() {
    float sum = 0;
    for(int i = 0; i < BUFFER_SIZE; i++)
        sum+= Buffer[i];
    
    float i = sum / (float)BUFFER_SIZE;  
    
    // Round 1mA -> 0
    return i > 1.51 ? i : 0;
}