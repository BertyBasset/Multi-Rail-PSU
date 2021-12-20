// Issues
// 1. +3.3 and +5V channels come from same regulator.
//    Currently we are splitting soft trip between them whith hard values rather than soft tripping on the sum 
// 2. -15V channel CS amp has +50mA offset. Therefore we have to remove this offset for display,
//    and to be on the safe side, derate soft trip by -50mA
// 3. -ve current sense are quite inacurrate, and are rounded to nearest 10mA
//    They are probably non-linear as well.
//    THere is also issue when >1 -ve rail is on, whereby approx +20mA of leakage appears on other channels - probably due to magnetic coupling




#include <Arduino.h>
#include <math.h>
#include "Rail.h"
#include "InputEvents.h"
#include "ThermalController.h"
#include <EEPROM.h>         // This is where we save selected preset number to make it non-volatile
#include "VDisplay.h"       // Using virtual display rather than LCD
#include "TripHistory.h"
#include <ADC.h>

#define res_bits 16

// Rail IDS
const int RAIL_ID_PLUS_3V3          = 0;
const int RAIL_ID_PLUS_FIVE         = 1;
const int RAIL_ID_PLUS_TWELVE       = 2;
const int RAIL_ID_PLUS_FIFTEEN      = 3;
const int RAIL_ID_MINUS_FIVE        = 4;
const int RAIL_ID_MINUS_TWELVE      = 5;
const int RAIL_ID_MINUS_FIFTEEN     = 6;

// Temperature Monitor/Control Thresholds
const int TEMPERATURE_FAN_ON        = 27;
const int TEMPERATURE_THERMAL_FAULT = 30;


// Pin Number Constant Declarations
//  Relays
const int PIN_RELAY_PLUS_3V3        = 8;
const int PIN_RELAY_PLUS_5V         = 7;
const int PIN_RELAY_PLUS_12V        = 6;
const int PIN_RELAY_PLUS_15V        = 5;
const int PIN_RELAY_MINUS_5V        = 4;
const int PIN_RELAY_MINUS_12V       = 3;
const int PIN_RELAY_MINUS_15V       = 2;

//  Fault LEDs
const int PIN_FAULT_LED_PLUS_3V3    = 36;
const int PIN_FAULT_LED_PLUS_5V     = 37;
const int PIN_FAULT_LED_PLUS_12V    = 38;
const int PIN_FAULT_LED_PLUS_15V    = 39;
const int PIN_FAULT_LED_MINUS_5V    = 13;
const int PIN_FAULT_LED_MINUS_12V   = 30;
const int PIN_FAULT_LED_MINUS_15V   = 29;

// V Sense analog inputs
const int PIN_VSENSE_PLUS_3V3       = A0;
const int PIN_VSENSE_PLUS_5V        = A1;
const int PIN_VSENSE_PLUS_12V       = A2;
const int PIN_VSENSE_PLUS_15V       = A3;
const int PIN_VSENSE_MINUS_5V       = A6;
const int PIN_VSENSE_MINUS_12V      = A7;
const int PIN_VSENSE_MINUS_15V      = A8;

// C Sense analog inputs
const int PIN_CSENSE_PLUS_3V3       = A13;
const int PIN_CSENSE_PLUS_5V        = A14;
const int PIN_CSENSE_PLUS_12V       = A15;
const int PIN_CSENSE_PLUS_15V       = A16;
const int PIN_CSENSE_MINUS_5V       = A4;
const int PIN_CSENSE_MINUS_12V      = A5;
const int PIN_CSENSE_MINUS_15V      = A9;

// Buttons
const int PIN_DISPLAY_BUTTON        = 10;
const int PIN_RAIL_BUTTON           = 11;

// Temperature
const int PIN_TEMPERATURE_SENSOR    = A12;
const int PIN_COOLING_FANS          = 9;

// LCD Display
const int PIN_LCD_RS                = 24;
const int PIN_LCD_EN                = 12;
const int PIN_LCD_D4                = 28;
const int PIN_LCD_D5                = 27;
const int PIN_LCD_D6                = 26;
const int PIN_LCD_D7                = 25;


enum PsuStates {
    off,
    on,
    thermalFault,
    railFault
};


void faultHandler(int, int);
void printRail(String, int);
void buttonHandler(int pin, int event);
void printStatusAndTemperature(int temperature);
void SplashScreen();
void SettingsScreen();
void DisplayPresetScreen();
void updateHistoryDisplay();

//LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
const int COLS = 20;
const int ROWS = 4;
const int PAGES = 4;


const int PAGE_MAIN = 0;
const int PAGE_HISTORY = 1;
const int PAGE_SETTINGS = 2;
const int PAGE_PRESETS = 3;
int currentPage = PAGE_MAIN;

VDisplay lcd = VDisplay(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7, COLS, ROWS, PAGES);
ADC* adc = new ADC();


// Soft Trip Values
const int TRIP_PLUS33 = 130;        // | These two share the same 630mA fuse, so make soft-trip at 580mA
const int TRIP_PLUS5 = 450;         // |   Rather than have a fixed split, we could do this dynamically
const int TRIP_PLUS12 = 450;        // Fuse 500mA
const int TRIP_PLUS15 = 300;        // Fuse 315mA
const int TRIP_MINUS5 =  550;       // Fuse 630mA
const int TRIP_MINUS12 = 450;       // Fuse 500mA
const int TRIP_MINUS15 = 250;       // Fuse 315mA - for some reason, we've got a 50mA offset on this, so need to 'derate' accordingly


// We have .6125A shared between 3.3V and 5V rails - eventually we need trip to accomodate the total across both channels                                                                         
// but for now, we''l allocate 130mA to 3.3V, and 450mA to 5V rail                                                                                                                                Calibration________ 
//                        Chan Name     Chan ID                Nom V  PinRelay             PinFaultLed              PinVSense             R1   R2   PinCSense             RSense   MaxFltCmA      IScale       IOffset   ADC  FaultHanlder  PrintHandler
Rail plus3v3 =      Rail(  "3.3 Volts", RAIL_ID_PLUS_3V3,        3.3, PIN_RELAY_PLUS_3V3,  PIN_FAULT_LED_PLUS_3V3,  PIN_VSENSE_PLUS_3V3,  888, 800, PIN_CSENSE_PLUS_3V3,  0.051,   TRIP_PLUS33,   0.7690140,   0.0,      adc, faultHandler, printRail);
Rail plusFive =     Rail(  "5 Volts",   RAIL_ID_PLUS_FIVE,       5,   PIN_RELAY_PLUS_5V,   PIN_FAULT_LED_PLUS_5V,   PIN_VSENSE_PLUS_5V,   160, 100, PIN_CSENSE_PLUS_5V,   0.051,   TRIP_PLUS5,    0.9150943,   0.0,      adc, faultHandler, printRail);
Rail plusTwelve =   Rail( "12 Volts",   RAIL_ID_PLUS_TWELVE,    12,   PIN_RELAY_PLUS_12V,  PIN_FAULT_LED_PLUS_12V,  PIN_VSENSE_PLUS_12V,  388, 100, PIN_CSENSE_PLUS_12V,  0.063,   TRIP_PLUS12,   1.0590406,   0.0,      adc, faultHandler, printRail);
Rail plusFifteen =  Rail( "15 Volts",   RAIL_ID_PLUS_FIFTEEN,   15,   PIN_RELAY_PLUS_15V,  PIN_FAULT_LED_PLUS_15V,  PIN_VSENSE_PLUS_15V,  472, 100, PIN_CSENSE_PLUS_15V,  0.100,   TRIP_PLUS15,   0.8967626,  -0.051,    adc, faultHandler, printRail);
Rail minusFive =    Rail( "-5 Volts",   RAIL_ID_MINUS_FIVE,     -5,   PIN_RELAY_MINUS_5V,  PIN_FAULT_LED_MINUS_5V,  PIN_VSENSE_MINUS_5V,  160, 100, PIN_CSENSE_MINUS_5V,  0,       TRIP_MINUS5,   9.8607417, -25.03,     adc, faultHandler, printRail);
Rail minusTwelve =  Rail("-12 Volts",   RAIL_ID_MINUS_TWELVE,  -12,   PIN_RELAY_MINUS_12V, PIN_FAULT_LED_MINUS_12V, PIN_VSENSE_MINUS_12V, 395, 100, PIN_CSENSE_MINUS_12V, 0,       TRIP_MINUS12,  17.354210, -44.06,     adc, faultHandler, printRail);
Rail minusFifteen = Rail("-15 Volts",   RAIL_ID_MINUS_FIFTEEN, -15,   PIN_RELAY_MINUS_15V, PIN_FAULT_LED_MINUS_15V, PIN_VSENSE_MINUS_15V, 472, 100, PIN_CSENSE_MINUS_15V, 0,       TRIP_MINUS15,  17.597036, -44.67,     adc, faultHandler, printRail);

Rail rails[] = {plus3v3, plusFive, plusTwelve, plusFifteen, minusFive, minusTwelve, minusFifteen};

PsuStates psuState = PsuStates::off;

InputEvents inputEvents = InputEvents();


const char DEGREE_SYMBOL = (char)223;
ThermalController thermalController = ThermalController(PIN_TEMPERATURE_SENSOR, PIN_COOLING_FANS, TEMPERATURE_FAN_ON, TEMPERATURE_THERMAL_FAULT, faultHandler);
bool fanState = false;


const int NUM_PRESETS = 11;
// Preset Stuff
//                       n/a  -15  -12  -5   +15  +12  +5   +3.3 
//                  bit  7    6    5    4    3    2    1    0    
char presets[11] = {                                               
    0b00000010,    //                                  +5V              0
    0b00000001,    //                                       +3.3V       1   
    0b00000100,    //                             +12V                  2
    0b00001000,    //                        +15V                       3
    0b00000011,    //                                  +5V  +3V         4
    0b00010011,    //                   -5V            +5V  +3V         5
    0b00100100,    //              -12V           +12V                  6
    0b01001000,    //         -15V           +15V                       7
    0b00100110,    //              -12V           +12V +5V              8
    0b01001010,    //         -15V           +15V      +5V              9
    0b01111111,    //         -15V -12V -5V  +15V +12V +5V  +3.3V       10

};
const int EEPROM_SELECTED_PRESET_ADDRESS = 0;
int selectedPreset;

TripHistory tripHistory = TripHistory();


void setup() {
    adc->adc0->setResolution(res_bits);
    adc->adc1->setResolution(res_bits);

    // If we've not previously writtem to Eeprom, need to default preset to 0
    selectedPreset = EEPROM.read(EEPROM_SELECTED_PRESET_ADDRESS);
    if(selectedPreset >= NUM_PRESETS)
        selectedPreset = 0;

    SplashScreen();
    SettingsScreen();

    // Display what preset we are on (as we are saving setting to flash)
    DisplayPresetScreen();
    delay(2000);

    lcd.setTargetPage(PAGE_HISTORY);
    lcd.setCursor(11,0);
    lcd.print("History:");

    lcd.setTargetPage(PAGE_PRESETS);
    lcd.setCursor(11,0);
    lcd.print("Presets:");

    lcd.setTargetPage(currentPage);


    inputEvents.registerDigitalInputEvent(PIN_DISPLAY_BUTTON, buttonHandler, HIGH);
    inputEvents.registerDigitalInputEvent(PIN_RAIL_BUTTON, buttonHandler, HIGH);

    pinMode(PIN_COOLING_FANS, OUTPUT);
}


unsigned long timeOffset = millis();
unsigned long timeOffset2 = millis();

void loop() {
    inputEvents.processEvents();

    for (byte i = 0; i < sizeof(rails)/sizeof(*rails); i++) {
        rails[i].Main();
    }

    if(millis() - timeOffset2 > 10) {
        thermalController.Main();
    }

    if(millis() - timeOffset > 1000) {
        printStatusAndTemperature(thermalController.temperature);
        timeOffset = millis();
    }
}


void buttonHandler(int pin, int event) {
    switch (pin) {
        case PIN_DISPLAY_BUTTON:
            switch (event) {
                case InputEvents::HIGH_TO_LOW:
                    currentPage ++;
                    if(currentPage > PAGE_HISTORY)
                        currentPage = 0;
                    lcd.setVisiblePage(currentPage);
                break;
            case InputEvents::HELD_HIGH:
                    if(currentPage == PAGE_SETTINGS)
                        currentPage = PAGE_MAIN;
                    else
                        currentPage = PAGE_SETTINGS;
                    lcd.setVisiblePage(currentPage);
                break;
            }
        break;
        case PIN_RAIL_BUTTON:
            switch (event) {
                case InputEvents::HIGH_TO_LOW:
                    if(currentPage != PAGE_PRESETS) {
                        if(psuState == off) {
                            // As we are powering up, go to Main page if not there
                            if(currentPage != PAGE_MAIN) {
                                currentPage = PAGE_MAIN;
                                lcd.setVisiblePage(currentPage);
                            }

                            psuState = on;
                            char mask = 0b0000001;

                            for (byte i = 0; i < sizeof(rails)/sizeof(*rails); i++) {
                                // Mask with preset
                                if((mask & presets[selectedPreset]) != 0)                    
                                    rails[i].setState(Rail::RailStates::on);
                                else    
                                    rails[i].setState(Rail::RailStates::off);

                                mask = mask << 1;
                            }
                        } else {
                            psuState = off;
                            for (byte i = 0; i < sizeof(rails)/sizeof(*rails); i++) {
                                rails[i].setState(Rail::RailStates::off);
                            }
                        }
                    } else {
                        // Go through presets - HOLD selects and returns

                        selectedPreset++;
                        if(selectedPreset >= NUM_PRESETS)
                            selectedPreset = 0;

                        DisplayPresetScreen();
                    }
                    break;
                case InputEvents::HELD_HIGH:
                    if(currentPage != PAGE_PRESETS && psuState == off) {   // Can only go to presets if PSU is off
                        currentPage = PAGE_PRESETS;
                        DisplayPresetScreen();
                    } else {
                        // We are on Preset page
                        // We are leaving preset page, so save selected preset to Eeprom
                        EEPROM.write(EEPROM_SELECTED_PRESET_ADDRESS, selectedPreset);

                        currentPage = PAGE_MAIN;
                    }
                    lcd.setVisiblePage(currentPage);
                break;
            }
        break;
    }
}


void DisplayPresetScreen() {
    lcd.setTargetPage(currentPage);
    lcd.setVisiblePage(currentPage);
    lcd.clear();
    lcd.setCursor(8, 0);
    lcd.print("Preset ");
    lcd.print(selectedPreset + 1);
    lcd.print("/");
    lcd.print(NUM_PRESETS);
    
    lcd.setCursor(0, 0);
    if((presets[selectedPreset] & 0b00000001) != 0)
        lcd.print(" +3.3V");
    lcd.setCursor(0, 1);
    if((presets[selectedPreset] & 0b00000010) != 0)
        lcd.print(" +5.0V");
    lcd.setCursor(0, 2);
    if((presets[selectedPreset] & 0b00000100) != 0)
        lcd.print("+12.0V");
    lcd.setCursor(0, 3);
    if((presets[selectedPreset] & 0b00001000) != 0)
        lcd.print("+15.0V");
    lcd.setCursor(14, 1);
    if((presets[selectedPreset] & 0b00010000) != 0)
        lcd.print(" -5.0V");
    lcd.setCursor(14, 2);
    if((presets[selectedPreset] & 0b00100000) != 0)
        lcd.print("-12.0V");
    lcd.setCursor(14, 3);
    if((presets[selectedPreset] & 0b01000000) != 0)
        lcd.print("-15.0V");
}


void faultHandler(int id, int tripCurrentMs) {
    tripHistory.addFault(id, tripCurrentMs);
    updateHistoryDisplay();

    if(id == -1 && psuState != thermalFault) {
        psuState = thermalFault;
        for (byte i = 0; i < sizeof(rails)/sizeof(*rails); i++) {
            rails[i].setState(Rail::RailStates::interiorFault);
        }
    }else if(id >= 0) {
        psuState = railFault;
        for (byte i = 0; i < sizeof(rails)/sizeof(*rails); i++) {
            if(i == id) {
                rails[i].setState(Rail::RailStates::interiorFault);
            }else {
                rails[i].setState(Rail::RailStates::exteriorFault);
            }
        }
    }

    // Show History Page
    currentPage = PAGE_HISTORY;
    lcd.setVisiblePage(currentPage);
}


// id=-1  for temperature
void printRail(String text, int id) {
    lcd.setTargetPage(PAGE_MAIN);
    lcd.setTargetPage(0);
    if(id == 0) {
        lcd.setCursor(0, 0);
    }else {
        //Assign id to grid
        lcd.setCursor(10 * (int)(id/4), id%3 + 3*!(id%3));
    }

    lcd.print(text);
}


// Special case here
long flashLastChanged = millis();
bool flashState = false;

void printStatusAndTemperature(int temperature) {
    lcd.setTargetPage(PAGE_MAIN);
    
    if(millis() - flashLastChanged > 500) {
        flashState = !flashState;
        flashLastChanged = millis();
    }

    // Temperature
    lcd.setCursor(15, 0);
    // Display flashing * if within 5% - 2°C of Temperature Fault trip
    if(temperature > TEMPERATURE_THERMAL_FAULT - (float)TEMPERATURE_THERMAL_FAULT * .05 - 2) {
        if(flashState) {
            lcd.print(temperature);
            lcd.print(DEGREE_SYMBOL);
            lcd.print("C*");
        } else
            lcd.print("     ");
    } else {
        lcd.print(temperature);
        lcd.print(DEGREE_SYMBOL);
        lcd.print("C ");
    }

    // Status
    lcd.setCursor(10, 0);
    if(psuState == off) {
        lcd.print(" Off ");
    } else if (psuState == on) {
        lcd.print("  On ");
    } else if (psuState == thermalFault) {
        if(flashState)
            lcd.print(" *T* ");
        else
            lcd.print("     ");
    } else if (psuState == railFault) {
        if(flashState)
            lcd.print(" *I* ");
        else
            lcd.print("     ");
    }
}


void SplashScreen() {
    lcd.setTargetPage(PAGE_MAIN);
    
    //         01234567890123456789
    lcd.print("     Multi Rail     ");
    lcd.setCursor(0, 1);
    lcd.print("    Power Supply    ");
    lcd.setCursor(0, 2);
    lcd.print("      (c) 2021      ");
    lcd.setCursor(0, 3);
    lcd.print("Shed Industries Inc.");
    delay(1000);
    lcd.clear();
}


void SettingsScreen() {
    lcd.setTargetPage(PAGE_SETTINGS);

    lcd.setCursor(0, 0);
    lcd.print("+3.3 ");
    lcd.print(TRIP_PLUS33);
    lcd.print("mA");
    lcd.setCursor(0, 1);
    lcd.print("  +5 ");
    lcd.print(TRIP_PLUS5);
    lcd.print("mA");
    lcd.setCursor(0, 2);
    lcd.print(" +12 ");
    lcd.print(TRIP_PLUS12);
    lcd.print("mA");
    lcd.setCursor(0, 3);
    lcd.print(" +15 ");
    lcd.print(TRIP_PLUS15);
    lcd.print("mA");

    lcd.setCursor(11, 1);
    lcd.print(" -5 ");
    lcd.print(TRIP_MINUS5);
    lcd.print("mA");
    lcd.setCursor(11, 2);
    lcd.print("-12 ");
    lcd.print(TRIP_MINUS12);
    lcd.print("mA");
    lcd.setCursor(11, 3);
    lcd.print("-15 ");
    lcd.print(TRIP_MINUS15);
    lcd.print("mA");

    lcd.setCursor(11, 0);
    lcd.print("*");
    lcd.print(TEMPERATURE_FAN_ON);
    lcd.print(DEGREE_SYMBOL);
    lcd.print(" ");

    lcd.print("!");
    lcd.print(TEMPERATURE_THERMAL_FAULT);
    lcd.print(DEGREE_SYMBOL);
}


void updateHistoryDisplay() {
    lcd.setTargetPage(PAGE_HISTORY);
    lcd.clear();
    lcd.setCursor(10, 0);
    lcd.print("History:");

    for(int i = 0; i < tripHistory.MAX_ITEMS; i++) {
        if(i < 4)
            lcd.setCursor(0, i);
        else
            lcd.setCursor(10, i - 3);
        
        if(tripHistory.tripHistory[i].channelID != -1) {
            switch(tripHistory.tripHistory[i].channelID) {
                case 0:
                    lcd.print("+3V3"); break;
                case 1:
                    lcd.print("+5V "); break;
                case 2:
                    lcd.print("+12V"); break;
                case 3:
                    lcd.print("+15V"); break;
                case 4:
                    lcd.print("-5V "); break;
                case 5:
                    lcd.print("-12V"); break;
                case 6:
                    lcd.print("-15V"); break;
            }

            if(tripHistory.tripHistory[i].tripCurrentMs < 100)
                lcd.print(" ");    
            if(tripHistory.tripHistory[i].tripCurrentMs < 10)
                lcd.print(" ");    

            lcd.print(tripHistory.tripHistory[i].tripCurrentMs);
            lcd.print("mA");
        }
    }
}