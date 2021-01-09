///// Project: 14 Band Spectrum Analyzer using WS2812B addressable "Smart" LED's and MSGEQ7 band-slicing IC's
///// Programmed and tested by Daniel Perez, A.K.A GeneratorLabs
///// Location: Myrtle Beach, South Carolina, United States of America
//N// E-Mail: generatorlabs@gmail.com
//O// Date: June 01, 2019
//E// Revision: Ver 2.3
//T// Target Platform: Arduino Mega2650 with SpeckyBoard
//E// License: This program is free software. You can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//S// the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
///// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
///// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
///// Credits: See acknowledgements & credits section below
/*
 ///// More information about this project, the necessary circuits and a source for parts-kits can be obtained by email (generatorlabs@gmail.com)
 ///// The notes & comments do not consume Arduino memory. They automatically get stripped out before compiled program is uploaded to Arduino.
 ///// Please keep notes in tact for future reference.
 /////
 ///// --- CREDITS & ACKNOWLEDGEMENTS ---
 ///// This sketch is based on a similar project and code from Denis Lanfrit and I respectfully thank Denis for his open source efforts.
 //N// The original code has been modified to allow a scalable platform with many more bands.
 //O// The library Si5351mcu is being utilized for programming masterclock IC frequencies. Special thanks to Pavel Milanes for his outstanding work. (https://github.com/pavelmc/Si5351mcu)
 //T// The library "FastLED" is being utilized to send data to the LED string. This library was developed by Daniel Garcia in 2012 and I respectfully
 //E// thank him for the amazing work as well. (https://github.com/FastLED/FastLED)
 //S//
 ///// This sketch is written for an Arduino Mega2650. While it is possible to run modified code on a UNO or NANO, the memory limitations of those
 ///// devices makes it impractical for a spectrum analyzer operating with more than 7 bands. The cost difference between the UNO and Mega2560 is so small that
 ///// it makes no sense to cram code into an UNO with only a few bytes to spare.
 /////
 //N// --- PIN ASSIGNMENTS ---
 //O//
 //T// Smart LED's use Pin 36 for data. The LED's are defined as a single wire WS2812B type in "SETUP" section.
 //E// Strobe signal uses pin 7
 //S// Reset signal uses pin 6
 ///// Analog reading of MSGEQ7 IC1 and IC2 use pin A0 and A1 respectively.
 ///// Make sure all boards, sub-assemblies, LED strings, etc are all tied to a common ground. Failure to do so will result in erratic operation and
 ///// possible circuit or component failure. Treat smart LED's with respect. They are susceptable to electrostatic discharge and improper voltages & polarity!
 ///// This code is being offered AS-IS. End user assumes any responsibility associated with the use of this code.
 */

#include <si5351mcu.h>



#include <FastLED.h> // You must include FastLED version 3.002.006. This library allows communication with each LED

/* MACROS */
#define DEBUG

#ifdef DEBUG
#define DEBUG_DO(_x) (_x)
#else
#define DEBUG_DO(_x)
#endif

#define LENOF(_arr) ((sizeof(_arr)) / (sizeof((_arr)[0])))

/* These definitions don't do nothing, it's just to visually see what we mean in different pin assignments */
#define ANALOG(x)   (x)
#define DIGITAL(x)  (x)

/* PINS */
#define RESET_PIN   DIGITAL(2)      // Pin to instruct MSGEQ7 to return to band zero and inspect it. Default Pin is 7. Default Pin on SpeckyBoard is 6.
#define STROBE_PIN  DIGITAL(3)      // Pin to instruct MSGEQ7 IC's to inspect next band (band 0 thru 6). Default Pin is 6. Default Pin on SpeckyBoard is 7.
#define HBEAT_PIN   DIGITAL(4)      // Pin for the hearbteat LED
#define DATA_PIN    DIGITAL(5)      // Pin for serial communication with LED string. This pin directs data thru termination resistor R13 on my 'SPECKY-BOARD'.
#define MSGEQ0_PIN  ANALOG(0)
#define MSGEQ1_PIN  ANALOG(1)
#define BRIGHT_PIN  ANALOG(2)
#define SMOOTH_PIN  ANALOG(3)

/* EQ CONFIG */
#define EQ_BANDS    (14)
#define NOISECOMP   (65)
#define EQ_DELTA    (30)

/* LED CONFIG */
#define ROWS                (24)
#define COLUMNS             (1)
#define DEFAULT_BRIGHT      (3)
#define BRIGHT_HYSTERESIS   (5)

enum color_effect {
    DEFAULT_EFFECT
};


/* GLOBAL VARIABLES */
Si5351mcu Si;
int MSGEQ_Bands[EQ_BANDS] = {0};
bool lit_matrix[COLUMNS][ROWS];
struct CRGB color_matrix[COLUMNS][ROWS];
struct CRGB fastLED_matrix[COLUMNS*ROWS];

int curr_brightness = DEFAULT_BRIGHT;

/* ----------------------------------- SETUP ----------------------------------- */

void setup()
{
    DEBUG_DO(Serial.begin(115200));
    DEBUG_DO(Serial.println("Start of init"));

    /* Clock generator init */
    Si.init(25000000L);
    Si.setFreq(0, 165000);
    Si.setFreq(1, 104000);
    Si.setPower(0, SIOUT_8mA);
    Si.setPower(1, SIOUT_8mA);
    Si.enable(0);
    Si.enable(1);

    /* FastLED library init */
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(fastLED_matrix, LENOF(fastLED_matrix));
    updateBrightness(DEFAULT_BRIGHT);

    /* pin configurations */
    pinMode(HBEAT_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STROBE_PIN, OUTPUT);
    pinMode(RESET_PIN, OUTPUT);

    /* LED matrix init */
    updateColorMatrix(DEFAULT_EFFECT);

    DEBUG_DO(Serial.println("init completed"));
}

/* ----------------------------------- LOOP ----------------------------------- */

unsigned long loop_counter = 0;
void loop()
{
    checkAndUpdateBrightness();
    readMSGEQ7();
    updateLEDMatrix();

    digitalWrite(HBEAT_PIN, HIGH);
    delay(20);
    digitalWrite(HBEAT_PIN, LOW);
    delay(10);

    ++loop_counter;
    if (1000000 == loop_counter)
        loop_counter = 0;
}

/* ----------------------------------- FUNCTIONS ----------------------------------- */

void checkAndUpdateBrightness()
{
    int new_brightness = min(100, analogRead(BRIGHT_PIN) / 10);
    updateBrightness(new_brightness);

    // if ((new_brightness > curr_brightness + BRIGHT_HYSTERESIS) ||
    //     (new_brightness < curr_brightness - BRIGHT_HYSTERESIS)) {
    //     updateBrightness(new_brightness);
    // }
}

void updateBrightness(int brightness) 
{
    curr_brightness = brightness;
    FastLED.setBrightness(brightness);
}

void updateColorMatrix(enum color_effect effect) 
{
    switch (effect) {
    case DEFAULT_EFFECT:
        updateColorMatrixDefaultEffect();
        break;
    default:
        DEBUG_DO(Serial.print("ERROR! got to updateColorMatrix with effect num: "));
        DEBUG_DO(Serial.println(effect));

        updateColorMatrixDefaultEffect();
        break;
    }
}

void updateColorMatrixDefaultEffect()
{
    for (int col = 0; col < COLUMNS; ++col) {
        for (int row = 0; row < ROWS; ++row) {
            color_matrix[col][row] = (row < 18) ? CRGB::Cyan : CRGB::Magenta;
        }
    }
}

void updateLEDMatrix()
{
    for (int col = 0; col < COLUMNS; ++col) {
        int height = getColumnHeight(col);
        if (shouldPrint()) {
            Serial.print("Column height: ");
            Serial.println(height);
        }
        for (int row = 0; row < ROWS; ++row) {
            lit_matrix[col][row] = (row < height);
        }
    }

    for (int col = 0; col < COLUMNS; ++col) {
        for (int row = 0; row < ROWS; ++row) {
            int flat_index = (col * ROWS) + row;
            fastLED_matrix[flat_index] =
                lit_matrix[col][row] ? color_matrix[col][row] : CRGB::Black;
        }
    }

    FastLED.show();
}

/* TODO: THIS WILL NEED TO BE CHANGED ACCRODING TO AMOUNT OF COLUMNS */
// int getColumnHeight(int column) {
//     return min(ROWS, MSGEQ_Bands[8] / EQ_DELTA);
// }

float prev_column_values[COLUMNS] = {0};
#define EQ_ALPHA    (float)(0)
int getColumnHeight(int column) {
    /* This is the (sort of) VU equivalent version */
    int prev_sum = 0;
    int sum = 0;
    float current, previous, value = 0;
    float alpha = mapToFloat(analogRead(SMOOTH_PIN), 0, 1023, 0.06, 1.0);

    for (int i = 0; i < EQ_BANDS; ++i) {
        sum += MSGEQ_Bands[i];
    }

    previous = prev_column_values[column];
    current = (((float)sum / 14.0) / 5.0);
    value = (current * alpha) + (prev_column_values[column] * (1.0-alpha));
    prev_column_values[column] = value;

    if (shouldPrint()) {
        Serial.print("alpha: ");Serial.println(alpha);
        Serial.print("previous: ");Serial.println(prev_column_values[column]);
        Serial.print("current: ");Serial.println(current);
        Serial.print("value: ");Serial.println(value);
    }
    return min(ROWS, (int)value);
}

void readMSGEQ7(void)
{
    digitalWrite(RESET_PIN, HIGH); // Part 1 of Reset Pulse. Reset pulse duration must be 100nS minimum.
    digitalWrite(RESET_PIN, LOW);  // Part 2 of Reset pulse. These two events consume more than 100nS in CPU time.

    DEBUG_DO(printBandPrologue());

    for (int band = 0; band < EQ_BANDS; band++)
    {                                                  // Loop that will increment counter that AnalogRead uses to determine which band to store data for.
        digitalWrite(STROBE_PIN, LOW);                 // Re-Set Strobe to LOW on each iteration of loop.
        delayMicroseconds(30);                         // Necessary delay required by MSGEQ7 for proper timing.
        MSGEQ_Bands[band] = max(0, analogRead(MSGEQ0_PIN) - NOISECOMP);
        DEBUG_DO(printBandValue(band, MSGEQ_Bands[band]));

        ++band;
        MSGEQ_Bands[band] = max(0, analogRead(MSGEQ1_PIN) - NOISECOMP);
        DEBUG_DO(printBandValue(band, MSGEQ_Bands[band]));

        digitalWrite(STROBE_PIN, HIGH);
    }
}

bool shouldPrint()
{
    return (0 == (loop_counter % 25));
}

void printBandPrologue()
{
    if (shouldPrint()) {
        Serial.print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        Serial.print("--");
        Serial.print(loop_counter);
        Serial.print("-- Brightness: ");
        Serial.print(curr_brightness);
        Serial.println();
    }
}

void printBandValue(int band, int value)
{
    if (shouldPrint()) {
        Serial.print(band);
        if (band < 10)
            Serial.print(": ");
        else
            Serial.print(":");
        Serial.print(MSGEQ_Bands[band]);
        Serial.println();
    }
}

float mapToFloat(int value, int in_min, int in_max, float out_min, float out_max) {
    return ((float)value - (float)in_min) * ((float)out_max - (float)out_min) / ((float)in_max - (float)in_min) + (float)out_min;
}
