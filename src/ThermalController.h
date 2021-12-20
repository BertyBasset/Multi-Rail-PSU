#ifndef _ThermalController_h
#define _ThermalController_h
#endif

#include "Arduino.h"

class ThermalController {

    public:
        ThermalController(int, int, int, int, void (*faultHandler)(int, int));

        void Main();

        int temperature;

    private:
        int pinSensor;
        int pinFans;
        int thresholdTemperature;
        int faultTemperature;
        int bufferIndex;
        int bufferSize;

        float temperatureBuffer[200];

        void (*raiseFault)(int, int);

};
