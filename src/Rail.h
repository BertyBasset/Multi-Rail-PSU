#ifndef _Rail_h
#define _Rail_h
#endif

#include "LED.h"
#include "Arduino.h"
#include "Sense\VSense.h"
#include "Sense\ISenseRes.h"
#include "Sense\ISenseHall.h"
#include "Sense\ISense.h"


class Rail {

    public:

        Rail(String, int, int, int, int, int, int, int, int, float, int, float, float, ADC *adc, void (*faultHandler)(int, int), void (*printText)(String, int));

        enum RailStates {
            off,
            on,
            interiorFault,
            exteriorFault
        };

        String displayText;

        void Main();
        void setState(RailStates);

        enum Fault {
            none,
            voltage,
            current,
        };

    private:
        RailStates currentState;
        String channelName;
        int id;
        int nominalVoltage;
        int pinRelay;
        int pinVSense;
        int pinISense;
        float iSense;
        float vSense;
        uint64_t displayTimeOffset;
        int maxCurrentmA;

        void (*printText)(String, int);
        void (*raiseFault)(int, int);

        LED faultLED;
        VSense* voltmeter;
        ISense* ammeter;

        void readVoltage();
        void readCurrent();
        void generateDisplayText();
        void Print(String);
};
