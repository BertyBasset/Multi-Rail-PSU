#ifndef _LED_h
#define _LED_h
#endif

#include "Arduino.h"

class LED {

    public:
        LED();
        
        LED(int);

        enum States {
            on,
            off,
            fading,
            flashing
        };

        void Main();
        void SetState(States, int period = 1000);

    private:

        States state;
        int pinLED;

        uint32_t period;
        uint64_t timeOffset;
        float dutyCycle;
        uint32_t timeDelay;
        bool fadeState;
};