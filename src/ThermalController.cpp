#include "ThermalController.h"

ThermalController::ThermalController(int pinSensor, int pinFans, int thresholdTemperature, int faultTemperature, void (*faultHandler)(int, int)) {

    ThermalController::pinSensor = pinSensor;
    ThermalController::pinFans = pinFans;
    ThermalController::thresholdTemperature = thresholdTemperature;
    ThermalController::faultTemperature = faultTemperature;

    ThermalController::raiseFault = faultHandler;

    bufferSize = sizeof(temperatureBuffer)/sizeof(*temperatureBuffer);

    for (byte i = 0; i < bufferSize; i++) {
        temperatureBuffer[i] = 20;
    }
    bufferIndex = 0;

    pinMode(pinFans, OUTPUT);

}


void ThermalController::Main() {
    //temperature = 165.0 / 512.0 * analogRead(pinSensor) - 50.0;

    // We are now using 16 bit res

    temperatureBuffer[bufferIndex] = 165.0 / 512.0 * (analogRead(pinSensor) / 64.0) - 50.0;
    bufferIndex = bufferIndex < bufferSize - 1 ? bufferIndex + 1 : 0;

    float sum = 0;
    for (byte i = 0; i < bufferSize; i++) {
        sum += temperatureBuffer[i];
    }
    temperature = sum/(float)bufferSize;

    // Little bit of hysteresis here (2V) by the looks of it. Consider +/-2 maybe?
    if(temperature > thresholdTemperature + 1) {
        digitalWrite(pinFans, HIGH);
    }else if(temperature < thresholdTemperature - 1) {
        digitalWrite(pinFans, LOW);
    }

    if(temperature > faultTemperature) {
        raiseFault(-1, -1);
    }
}
