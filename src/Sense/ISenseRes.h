#pragma once

#include <ADC.h>
#include "ISense.h"

#define VREF 3.3
#define res_bits 16
#define resolution (VREF/((pow(2,res_bits))-1));

class ISenseRes : public ISense {
    public:
        ISenseRes();
        ISenseRes(ADC* adc, int analogPin, float rSense, float iSscale, float iOffset);

        virtual int read();         // Returns current reading, or average if there's a cyclic buffer
        virtual void update();      // If there's cyclic avereging buffer, this will maintaint it

    private:
        float gain = 100.0;       // Gain of MAX437x output stage
        float iScale;
        float iOffset;

        int analogPin;
        float rSense;           // Current Sense resistor
        ADC *adc;

        constexpr static int BUFFER_SIZE = 200;
        float Buffer[BUFFER_SIZE];
        int bufferIndex = 0;    
};

