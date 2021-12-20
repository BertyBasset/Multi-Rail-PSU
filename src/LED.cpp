#include "LED.h"
#include "math.h"

LED::LED() {
    
}


LED::LED(int pin) {
    pinMode(pin, OUTPUT);
    LED::pinLED = pin;
    LED::state = off;

    timeOffset = micros();
    timeDelay = 0;
}


void LED::Main() {
    if(state == on) {
        digitalWrite(pinLED, HIGH);
    }

    if(state == flashing) {
        if(micros() - timeOffset > period) {
            digitalWrite(pinLED, LOW);
            timeOffset = micros();
        }else if(micros() - timeOffset > period/2) {
            digitalWrite(pinLED, HIGH);
        }
    }
    if(state == fading) {
        if(micros() - timeOffset > timeDelay) {
            dutyCycle = pow(sin(2*PI*micros()/period), 2);
            if(fadeState == true) {
                digitalWrite(pinLED, LOW);
                timeDelay = 10000 * dutyCycle;
            } else {
                digitalWrite(pinLED, HIGH);
                timeDelay = 10000 * (1 - dutyCycle);
            }
            fadeState = !fadeState;
            timeOffset = micros();
        }
    }
}


void LED::SetState(LED::States state, int period) {
    switch (state) {
        case on:
            //digitalWrite(pinLED, HIGH);     -- This now gets done in the loop
            LED::state = on;
        break;
        case off:
            digitalWrite(pinLED, LOW);
            LED::state = off;
        break;
        case flashing:
            LED::period = period * 1000;
            LED::state = flashing;
            timeOffset = micros();
        break;
        case fading:
            LED::period = period * 1000;
            LED::state = fading;
            fadeState = false;
        break;
    }
}
