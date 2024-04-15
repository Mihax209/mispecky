#include <Arduino.h>
#include <si5351mcu.h>

#include <common.h>
#include <spectrum.h>

/* Private functions decleration */
void printBandValue(int band, int value);
void printBandsPrologue();
void printBands();

/* Global variables */
Si5351mcu Si;
int MSGEQ_bands[EQ_BANDS] = {0};


void SPECTRUM_init()
{
    /* Clock generator init */
    /* NOTE: clock 0 is MSGEQ7 1 board-wise (oopsie) */
    Si.init(25000000L);
    Si.setFreq(0, 165000);
    Si.setFreq(1, 104000);
    Si.setPower(0, SIOUT_8mA);
    Si.setPower(1, SIOUT_8mA);
    Si.enable(0);
    Si.enable(1);
}


/* Read the MSGEQ7 values */
void SPECTRUM_sampleSpectrum()
{
    digitalWrite(RESET_PIN, HIGH); // Part 1 of Reset Pulse. Reset pulse duration must be 100nS minimum.
    digitalWrite(RESET_PIN, LOW);  // Part 2 of Reset pulse. These two events consume more than 100nS in CPU time.

    for (int band = 0; band < EQ_BANDS; band++)
    {                                                  // Loop that will increment counter that AnalogRead uses to determine which band to store data for.
        digitalWrite(STROBE_PIN, LOW);                 // Re-Set Strobe to LOW on each iteration of loop.
        delayMicroseconds(30);                         // Necessary delay required by MSGEQ7 for proper timing.
        MSGEQ_bands[band] = max(0, analogRead(MSGEQ0_PIN) - NOISECOMP) * GAIN;
        ++band;
        MSGEQ_bands[band] = max(0, analogRead(MSGEQ1_PIN) - NOISECOMP) * GAIN;

        digitalWrite(STROBE_PIN, HIGH);
    }

    DEBUG_DO(printBands());
}

int SPECTRUM_getBandValue(int band)
{
    return MSGEQ_bands[band];
}


void printBands()
{
    Serial.print("\n\n\n\n\n\n");
    Serial.print("--"); Serial.print(global_loop_counter); Serial.println("--");

    for (int band = 0; band < EQ_BANDS; ++band)
    {
        Serial.print(band);
        if (band < 10) {
            Serial.print(": ");
        }
        else {
            Serial.print(":");
        }

        Serial.println(MSGEQ_bands[band]);
    }
}
