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
Si5351mcu Si;

#include <FastLED.h> // You must include FastLED version 3.002.006. This library allows communication with each LED

#define DEBUG

#ifdef DEBUG
#define DEBUG_DO(_x) (_x)
#else
#define DEBUG_DO(_x)
#endif

/* PINS */
#define RESET_PIN   (2)     // Pin to instruct MSGEQ7 to return to band zero and inspect it. Default Pin is 7. Default Pin on SpeckyBoard is 6.
#define STROBE_PIN  (3)     // Pin to instruct MSGEQ7 IC's to inspect next band (band 0 thru 6). Default Pin is 6. Default Pin on SpeckyBoard is 7.
#define HBEAT_PIN   (4)     // Pin for the hearbteat LED
#define DATA_PIN    (5)     // Pin for serial communication with LED string. This pin directs data thru termination resistor R13 on my 'SPECKY-BOARD'.

/* EQ CONFIG */
#define COLUMNS     (14)
#define NOISECOMP   (200)

int MSGEQ_Bands[COLUMNS];    // Setup column array

/* ----------------------------------- SETUP ----------------------------------- */

void setup()
{
    DEBUG_DO(Serial.begin(115200));

    // Start Masterclock configuration; The remainder of this program will fail if this does not initialize!
    Si.init(25000000L);        // Library procedure to set up for use with non-default 25.000 MHz xtal
    Si.setFreq(0, 165000);     // Enable the output 0 with specified frequency of 165.000 KHz; Default Pin for SpeckyBoard
    Si.setFreq(1, 104000);     // Enable the output 1 with specified frequency of 104.000 KHz; Default Pin for SpeckyBoard
    Si.setPower(0, SIOUT_8mA); // Set power output level of clock 0
    Si.setPower(1, SIOUT_8mA); // Set power output level of clock 1
    Si.enable(0);              // Enable output 0
    Si.enable(1);              // Enable output 1
    // End Masterclock configuration

    pinMode(HBEAT_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STROBE_PIN, OUTPUT);
    pinMode(RESET_PIN, OUTPUT);
}

/* ----------------------------------- LOOP ----------------------------------- */

void loop()
{
    readMSGEQ7(); // Call to function that reads MSGEQ7 IC's via analogue inputs.


    digitalWrite(HBEAT_PIN, HIGH);
    delay(30);
    digitalWrite(HBEAT_PIN, LOW);
    delay(10);
}

/* ----------------------------------- FUNCTIONS ----------------------------------- */

unsigned long loop_counter = 0;
void readMSGEQ7(void) // Function that reads the 7 bands of the audio input.
{
    digitalWrite(RESET_PIN, HIGH); // Part 1 of Reset Pulse. Reset pulse duration must be 100nS minimum.
    digitalWrite(RESET_PIN, LOW);  // Part 2 of Reset pulse. These two events consume more than 100nS in CPU time.

    DEBUG_DO(Serial.println());DEBUG_DO(Serial.println());DEBUG_DO(Serial.println());DEBUG_DO(Serial.println());
    DEBUG_DO(Serial.println());DEBUG_DO(Serial.println());DEBUG_DO(Serial.println());DEBUG_DO(Serial.println());

    DEBUG_DO(Serial.print("--"));
    DEBUG_DO(Serial.print(loop_counter));
    DEBUG_DO(Serial.print("--"));
    DEBUG_DO(Serial.println());

    for (int band = 0; band < COLUMNS; band++)
    {                                                  // Loop that will increment counter that AnalogRead uses to determine which band to store data for.
        digitalWrite(STROBE_PIN, LOW);                 // Re-Set Strobe to LOW on each iteration of loop.
        delayMicroseconds(30);                         // Necessary delay required by MSGEQ7 for proper timing.
        MSGEQ_Bands[band] = analogRead(0) - NOISECOMP; // Saves the reading of the amplitude voltage on Analog Pin 0.

        DEBUG_DO(Serial.print(band));
        DEBUG_DO(Serial.print(":"));
        DEBUG_DO(Serial.print(MSGEQ_Bands[band]));
        DEBUG_DO(Serial.println());

        band++;
        MSGEQ_Bands[band] = analogRead(1) - NOISECOMP; // Saves the reading of the amplitude voltage on Analog Pin 1.

        DEBUG_DO(Serial.print(band));
        DEBUG_DO(Serial.print(":"));
        DEBUG_DO(Serial.print(MSGEQ_Bands[band]));
        DEBUG_DO(Serial.println());

        digitalWrite(STROBE_PIN, HIGH);
    }

    DEBUG_DO(Serial.println());

    ++loop_counter;
}
