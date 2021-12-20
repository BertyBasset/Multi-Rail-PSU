#include "Rail.h"
#include <Math.h>

                                                                                                                                                                                                          
Rail::Rail(String channelName, int id, int nominalVoltage, int pinRelay, int pinFaultLED, int pinVSense, int r1, int r2, int pinISense, float rSense, int maxCurrentmA, float iScale, float iOffset, ADC *adc, void (*faultHandler)(int, int), void (*printText)(String, int)) {
    Rail::channelName = channelName;
    Rail::nominalVoltage = nominalVoltage;
    Rail::pinRelay = pinRelay;
    Rail::pinVSense = pinVSense;
    Rail::pinISense = pinISense;
    Rail::id = id;
    Rail::maxCurrentmA = maxCurrentmA;

    displayTimeOffset = millis();

    pinMode(pinRelay, OUTPUT);
    pinMode(pinFaultLED, OUTPUT);

    Rail::printText = printText;
    Rail::raiseFault = faultHandler;

    faultLED = LED(pinFaultLED);

    voltmeter = new VSense(adc, pinVSense, r1, r2);

    // First 4 are resistor sense, rest are Hall effect
    if(id <= 3)
        ammeter = new ISenseRes(adc, pinISense, rSense, iScale, iOffset);
    else
        ammeter = new ISenseHall(adc, pinISense, iScale, iOffset);
}


void Rail::Main() {
    faultLED.Main();
    
    // The update methods don't return anything, they just pour data into the cyclic
    // buffer to be read as an average later using read
    ammeter->update();
    voltmeter->update();

    if(millis() - displayTimeOffset > 1000) {
        generateDisplayText();
        displayTimeOffset = millis();
    }
}


void Rail::readVoltage() {
    vSense = voltmeter->read();

    if(digitalRead(pinRelay) == LOW)
        vSense = 0;

    if(id >= 4 && vSense != 0)
        vSense = -vSense;
}


void Rail::readCurrent() {
    iSense = ammeter->read();

    if(digitalRead(pinRelay) == LOW)
        iSense = 0;

    if(abs(iSense) > maxCurrentmA)
        raiseFault(id, abs(iSense));        
}


void Rail::generateDisplayText() {
    readVoltage();
    readCurrent();

    // Ideally we could do with replacing this with a printf type function from stdlib
    // It'll do for now though

    // 01234567890123456789
    // -vv.v aaa -vv.v aaa
    String display = "";

    // Leading space
    if(abs(vSense) < 10)
        display+= " ";
    
    // Polarity symbol
    if(vSense < 0)
        display+= "-";
    else
        display+= " ";

    float value = abs(round(vSense * 10.0) / 10.0);
    // Int part
    display += (String)(int)value;
    display += ".";
    // Frac part
    display += (String)((int)((value - (int)value) * 10.0));

    display += " ";

    if(abs(iSense) < 100)
        display += " ";
    if(abs(iSense) < 10)
        display += " ";
    
    if(iSense < 1000)
        display += (String)((int)abs(iSense));
    else
        display += "???";       // Until we get scaling sorted, we can't display 1000mA ->

    display += "  ";
    
    displayText = display;
    Print(displayText);
}


void Rail::Print(String text) {
   printText(text, id);
}


void Rail::setState(Rail::RailStates state) {
    // For faults, we only set extrenal fault if the channel is on
    // If the channel is already off, just show as off

    switch (state) {
        case on:
            digitalWrite(pinRelay, HIGH);
            faultLED.SetState(LED::States::off);
        break;
        case off:
            digitalWrite(pinRelay, LOW);
            faultLED.SetState(LED::States::off);
        break;
        case interiorFault:
            digitalWrite(pinRelay, LOW);
            faultLED.SetState(LED::States::fading);
        break;
        case exteriorFault:
            digitalWrite(pinRelay, LOW);
            // If we are already off, don't show as faulted
            if(currentState == off) {
                faultLED.SetState(LED::States::off);
                state = RailStates::off;
            } else
                faultLED.SetState(LED::States::on);


    }
    currentState = state;
}
