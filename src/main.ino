#include <si5351mcu.h>
#include <FastLED.h> // You must include FastLED version 3.002.006. This library allows communication with each LED

#define DEBUG
/* MACROS */
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
#define EQ_DELTA    (40.0)

/* LED CONFIG */
#define ROWS                (24)
#define COLUMNS             (2)
#define DEFAULT_BRIGHT      (3)
#define BRIGHT_HYSTERESIS   (1)

enum color_effect {
    DEFAULT_EFFECT
};


/* GLOBAL VARIABLES */
Si5351mcu Si;
int MSGEQ_Bands[EQ_BANDS] = {0};
bool lit_matrix[COLUMNS][ROWS];
struct CRGB color_matrix[COLUMNS][ROWS];
struct CRGB fastLED_matrix[COLUMNS*ROWS];
float prev_column_values[COLUMNS] = {0};

int curr_brightness = DEFAULT_BRIGHT;

/* ----------------------------------- SETUP ----------------------------------- */

void setup()
{
    DEBUG_DO(Serial.begin(115200));
    DEBUG_DO(Serial.println("Start of init"));

    /* Clock generator init */
    /* NOTE: clock 0 is MSGEQ7 1 board-wise (oopsie) */
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
    int new_brightness = map(analogRead(BRIGHT_PIN), 0, 1023, 0, 100);

    if ((new_brightness > curr_brightness + BRIGHT_HYSTERESIS) ||
        (new_brightness < curr_brightness - BRIGHT_HYSTERESIS)) {
        updateBrightness(new_brightness);
    }

    updateBrightness(new_brightness);
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
int getColumnHeight(int column) {
    /* This is a 14 -> 12 band convertor */
    float current, previous, value = 0;
    float alpha = mapToFloat(analogRead(SMOOTH_PIN), 0, 1023, 1.0, 0.06);

    previous = prev_column_values[column];
    current = MSGEQ_Bands[column + 1];
    current /= EQ_DELTA;

    value = (current * alpha) + (prev_column_values[column] * (1.0-alpha));
    prev_column_values[column] = value;

    if (shouldPrint()) {
        DEBUG_DO(Serial.print("alpha: ")); DEBUG_DO(Serial.println(alpha));
        DEBUG_DO(Serial.print("previous: ")); DEBUG_DO(Serial.println(prev_column_values[column]));
        DEBUG_DO(Serial.print("current: ")); DEBUG_DO(Serial.println(current));
        DEBUG_DO(Serial.print("value: ")); DEBUG_DO(Serial.println(value));

        DEBUG_DO(Serial.println("--------------------------------"));
    }
    return min(ROWS, (int)value);
}

// int getColumnHeight(int column) {
//     /* This is a crude 2 band eq */
//     float current, previous, value = 0;
//     float alpha = mapToFloat(analogRead(SMOOTH_PIN), 0, 1023, 1.0, 0.06);

//     previous = prev_column_values[column];
//     current = (column == 0) ? MSGEQ_Bands[1] : MSGEQ_Bands[12];
//     current /= EQ_DELTA;
//     value = (current * alpha) + (prev_column_values[column] * (1.0-alpha));
//     prev_column_values[column] = value;

//     if (shouldPrint()) {
//         DEBUG_DO(Serial.print("alpha: ")); DEBUG_DO(Serial.println(alpha));
//         DEBUG_DO(Serial.print("previous: ")); DEBUG_DO(Serial.println(prev_column_values[column]));
//         DEBUG_DO(Serial.print("current: ")); DEBUG_DO(Serial.println(current));
//         DEBUG_DO(Serial.print("value: ")); DEBUG_DO(Serial.println(value));

//         DEBUG_DO(Serial.println("--------------------------------"));
//     }
//     return min(ROWS, (int)value);
// }

// int getColumnHeight(int column) {
//     /* This is the (sort of) VU equivalent version */
//     float current, previous, value = 0;
//     float alpha = mapToFloat(analogRead(SMOOTH_PIN), 0, 1023, 1.0, 0.06);

//     for (int i = 0; i < EQ_BANDS; ++i) {
//         sum += MSGEQ_Bands[i];
//     }

//     previous = prev_column_values[column];
//     current = (((float)sum / 14.0) / 5.0);
//     value = (current * alpha) + (prev_column_values[column] * (1.0-alpha));
//     prev_column_values[column] = value;

//     if (shouldPrint()) {
//         Serial.print("alpha: ");Serial.println(alpha);
//         Serial.print("previous: ");Serial.println(prev_column_values[column]);
//         Serial.print("current: ");Serial.println(current);
//         Serial.print("value: ");Serial.println(value);
//     }
//     return min(ROWS, (int)value);
// }

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
