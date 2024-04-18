#include <Arduino.h>
#include <FastLED.h>
#include <limits.h>

#include <common.h>
#include <commands.h>
#include <spectrum.h>
#include <led.h>

/* Function declarations */
void blinkHeartbeat();
void incGlobalLoopCounter();

/* Global variables */
unsigned long global_loop_counter = 0;

/* ----------------------------------- SETUP ----------------------------------- */

void setup()
{
    Serial.begin(115200);
    DEBUG_DO(Serial.println("Start of init"));

    SPECTRUM_init();

    LED_init();
    LED_setColorMatrix(INVERTED_COLOR);

    /* pin configurations */
    pinMode(HBEAT_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STROBE_PIN, OUTPUT);
    pinMode(RESET_PIN, OUTPUT);

    global_loop_counter = 0;

    DEBUG_DO(Serial.println("init completed"));
}

/* ----------------------------------- LOOP ----------------------------------- */

void loop()
{
    COMMANDS_checkSerialCommands();
    SPECTRUM_sampleSpectrum();
    LED_updateLEDMatrix();

    incGlobalLoopCounter();
}

void incGlobalLoopCounter()
{
    global_loop_counter++;
    if (ULONG_MAX == global_loop_counter) {
        global_loop_counter = 0;
    }
}

/* LEGACY, unused anymore */
void blinkHeartbeat()
{
    digitalWrite(HBEAT_PIN, HIGH);
    delay(20);
    digitalWrite(HBEAT_PIN, LOW);
    delay(10);
}
