#pragma once

#include <ADC.h>


#define VREF 3.3
#define res_bits 16
#define resolution (VREF/((pow(2,res_bits))-1));


class VSense { 
    public:
        VSense();
        VSense(ADC* adc, int analogPin, int r1, int r2);

        virtual double read();         // Returns current reading, or average if there's a cyclic buffer
        virtual void update();      // If there's cyclic avereging buffer, this will maintaint it

        
    private:
        int ADC_RESOLUTION = 4096;
        float V_REF = 3.3;

        int analogPin;
        float attenuation;
        ADC *adc;

        constexpr static int BUFFER_SIZE = 200;
        float Buffer[BUFFER_SIZE];
        int bufferIndex = 0;        
};
