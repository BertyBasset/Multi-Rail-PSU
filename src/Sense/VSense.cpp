#include <Arduino.h>
#include "VSense.h"

// Constructor
VSense::VSense() {}

VSense::VSense(ADC* adc, int analogPin, int r1, int r2) {
    VSense::adc = adc;
    VSense::analogPin = analogPin;
    VSense::attenuation = (float)r2 / (float)r1;
    pinMode(analogPin, INPUT);
}


// Maintain cyclic averaging buffer if there is one
void VSense::update() {
   float voltage= (float)(adc->analogRead(analogPin)) * resolution;
   float v = voltage / attenuation;

   bufferIndex++;
   if(bufferIndex >= BUFFER_SIZE)
      bufferIndex = 0;
    Buffer[bufferIndex] = v;
}

// Return value, or average from cyclic buffer
double VSense::read() {
    float sum = 0;
    for(int i = 0; i < BUFFER_SIZE; i++)
        sum+= Buffer[i];
    
    return sum / (float)BUFFER_SIZE;  
} 