#pragma once

#include <ADC.h>
#include "ISense.h"
#define VREF 3.3
#define res_bits 16
#define resolution (VREF/((pow(2,res_bits))-1));


class ISenseHall : public ISense {
    // Sensor is ACS712 5A
    // Data Sheet

    public:
        ISenseHall();
        ISenseHall(ADC* adc, int analogPin, float iSscale, float iOffset);

        virtual int read();         // Returns current reading, or average if there's a cyclic buffer
        virtual void update();      // If there's cyclic avereging buffer, this will maintain it, also passing last reading for tripping if necessary

 private:
        int analogPin;
        ADC *adc;

        float m;                  // y = mx + c
        float c;

        constexpr static int BUFFER_SIZE = 200;
        float Buffer[BUFFER_SIZE];
        int bufferIndex = 0;
};