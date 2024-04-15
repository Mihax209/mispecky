#include <Arduino.h>
#include <si5351mcu.h>
#include <FastLED.h>

#define DEBUG

/* MACROS */
#ifdef DEBUG
#define DEBUG_DO(_x) (_x)
#else
#define DEBUG_DO(_x)
#endif

#define PRINT_INTERVAL (60)

#define LENOF(_arr) ((sizeof(_arr)) / (sizeof((_arr)[0])))

/* These definitions don't do nothing, it's just to visually see what we mean in different pin assignments */
#define ANALOG(x)   (x)
#define DIGITAL(x)  (x)

/* PINS */
#define RESET_PIN   DIGITAL(2)      // Pin to instruct MSGEQ7 to return to band zero and inspect it
#define STROBE_PIN  DIGITAL(3)      // Pin to instruct MSGEQ7 IC's to inspect next band (band 0 thru 6)
#define HBEAT_PIN   DIGITAL(4)      // Pin for the hearbteat LED
#define DATA_PIN    DIGITAL(5)      // Pin for serial communication with LED string
#define MSGEQ0_PIN  ANALOG(0)
#define MSGEQ1_PIN  ANALOG(1)
#define BRIGHT_PIN  ANALOG(4)
#define SMOOTH_PIN  ANALOG(2)

/* EQ CONFIG */
#define EQ_BANDS    (14)
#define NOISECOMP   (70)
#define GAIN        (1.2)
#define EQ_DELTA    (5.0)

/* LED CONFIG */
#define ROWS                    (23)
#define COLUMNS                 (12)
#define AMOUNT_TRIMMED          (4)
#define DEFAULT_BRIGHT          (1)
#define MAX_BRIGHTNESS          (35)
#define PEAK_INDICATOR_TIMEOUT  (25)

enum color_effect {
    DEFAULT_COLOR,
    INVERTED_COLOR,
    VERTICAL_GRADIENT,
    HORITZONTAL_GRADIENT,
};

enum serial_commands {
    BRIGHTNESS_COMMAND='B'
};

/* GLOBAL VARIABLES */
Si5351mcu Si;
int MSGEQ_Bands[EQ_BANDS] = {0};
bool lit_matrix[COLUMNS][ROWS];
struct CRGB color_matrix[COLUMNS][ROWS];
struct CRGB fastLED_matrix[(COLUMNS*ROWS) - (2*AMOUNT_TRIMMED)];
uint8_t peak_indicators[COLUMNS] = {0};
unsigned int peak_indicators_timeout[COLUMNS] = {0};
float prev_column_values[COLUMNS] = {0};
bool g_peak_indicators_enabled = false;
int g_curr_bright_analog = 0;
int g_curr_brightness = DEFAULT_BRIGHT;

/* Function declarations */
void checkSerial();
void checkAndUpdateBrightness(String& value_str);
void updateBrightness(int brightness);
void setColorMatrix(enum color_effect effect) ;
void defaultColor();
void invertedColor();
void verticalGradientColor();
void horizontalGradientColor();
void updateLEDMatrix();

int getColumnHeight(int column);
void readMSGEQ7(void);
bool shouldPrint();
void printBandPrologue();
void printBandValue(int band, int value);
float mapToFloat(int value, int in_min, int in_max, float out_min, float out_max);

/* ----------------------------------- SETUP ----------------------------------- */

void setup()
{
    Serial.begin(115200);
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
    setColorMatrix(INVERTED_COLOR);

    DEBUG_DO(Serial.println("init completed"));
}

/* ----------------------------------- LOOP ----------------------------------- */

unsigned long loop_counter = 0;
void loop()
{
    checkSerial();
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

void checkSerial()
{
    if (Serial.available() <= 0) {
        return;
    }

    String input_string = Serial.readStringUntil('\n');

    if (input_string.charAt(1) != ' ') {
        Serial.println("ERROR: invalid command format");
        return;
    }

    char command_char = input_string.charAt(0);
    String command_data = input_string.substring(2);
    switch (command_char)
    {
    case BRIGHTNESS_COMMAND:
        checkAndUpdateBrightness(command_data);
        break;
    default:
        Serial.print("Invalid command: "); Serial.println(command_char);
    }
}

void checkAndUpdateBrightness(String& value_str)
{
    int value = atoi(value_str.c_str());
    if ((value > 100) || (value < 0)) {
        Serial.print("ERROR: brightness should be between 0 and 100 (got ");
        Serial.print(value); Serial.println(")");
        return;
    }

    int new_brightness = (int)map(value, 0, 100, 0, MAX_BRIGHTNESS);

    updateBrightness(new_brightness);
}

void updateBrightness(int brightness)
{
    g_curr_brightness = brightness;
    FastLED.setBrightness(brightness);
    DEBUG_DO(Serial.print("Brightness updated to: "));
    DEBUG_DO(Serial.println(brightness));
}

void setColorMatrix(enum color_effect effect) 
{
    switch (effect) {
    case DEFAULT_COLOR:
        defaultColor();
        break;
    case INVERTED_COLOR:
        invertedColor();
        break;
    case VERTICAL_GRADIENT:
        verticalGradientColor();
        break;
    case HORITZONTAL_GRADIENT:
        horizontalGradientColor();
        break;
    default:
        DEBUG_DO(Serial.print("ERROR! got to updateColorMatrix with effect num: "));
        DEBUG_DO(Serial.println(effect));

        defaultColor();
        break;
    }
}

void defaultColor()
{
    for (int col = 0; col < COLUMNS; ++col) {
        for (int row = 0; row < ROWS; ++row) {
            color_matrix[col][row] = 
                (row < 11) ? CRGB::Cyan : (row < 18 ? CRGB::Magenta : CRGB::Yellow);
        }
    }
}

void invertedColor()
{
    for (int col = 0; col < COLUMNS; ++col) {
        for (int row = 0; row < ROWS; ++row) {
            color_matrix[col][row] = 
                (row < 11) ? CRGB::Cyan : (row < 18 ? CRGB::Yellow : CRGB::Magenta);
        }
    }
}

void verticalGradientColor()
{
    int start_hue = 235;
    int end_hue = 5;

    int curr_hue = start_hue;
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLUMNS; ++col) {
            color_matrix[col][row] = CHSV((uint8_t)curr_hue, 255, 255);
        }

        curr_hue += (end_hue - start_hue) / ROWS;
    }
}

void horizontalGradientColor()
{
    int start_hue = 240;
    int end_hue = 0;

    int curr_hue = start_hue;
    for (int col = 0; col < COLUMNS; ++col) {
        for (int row = 0; row < ROWS; ++row) {
            color_matrix[col][row] = CHSV((uint8_t)curr_hue, 255, 255);
        }

        curr_hue += (end_hue - start_hue) / COLUMNS;
    }
}

void updateLEDMatrix()
{
    for (int col = 0; col < COLUMNS; ++col) {
        int height = min(getColumnHeight(col), ROWS);
        /* light up EQ band */
        for (int row = 0; row < ROWS; ++row) {
            lit_matrix[col][row] = (row < height);
        }

        if (g_peak_indicators_enabled) {
            /* add peak indicator */
            if (height > peak_indicators[col] ||
                peak_indicators_timeout[col] > PEAK_INDICATOR_TIMEOUT
            ) {
                peak_indicators[col] = height;
                peak_indicators_timeout[col] = 0;
                lit_matrix[col][peak_indicators[col]] = false;
            }
            else {
                peak_indicators_timeout[col]++;
                lit_matrix[col][peak_indicators[col]] = true;
            }
        }

        if (shouldPrint()) {
            DEBUG_DO(Serial.print("Column height: "));
            DEBUG_DO(Serial.print(height));
            DEBUG_DO(Serial.print(". Peak: "));
            DEBUG_DO(Serial.print(peak_indicators[col]));
            DEBUG_DO(Serial.print(" ("));
            DEBUG_DO(Serial.print(peak_indicators_timeout[col]));
            DEBUG_DO(Serial.println(")"));
        }
    }

    int ignore_cnt = 0;
    for (int col = 0; col < COLUMNS; ++col) {
        for (int row = 0; row < ROWS; ++row) {
            int flat_index = (col * ROWS) + row - AMOUNT_TRIMMED;

            if (flat_index >= 0) {
                fastLED_matrix[flat_index] =
                    lit_matrix[col][row] ? color_matrix[col][row] : CRGB::Black;
            }
        }
    }

    FastLED.show();
}

/* NOTE: THIS WILL NEED TO BE CHANGED ACCRODING TO AMOUNT OF COLUMNS */
int getColumnHeight(int column) {
    /* This is a 14 -> 12 band convertor */
    float current, previous, value = 0;
    float alpha = mapToFloat(analogRead(SMOOTH_PIN), 0, 1023, 1.0, 0.1);
    // float alpha = 0.4;

    previous = prev_column_values[column];
    current = MSGEQ_Bands[column + 1];
    if (column == 0) {
        // Bass average with invisible band
        current = max(current, MSGEQ_Bands[0]);
    }
    current /= EQ_DELTA;

    value = (current * alpha) + (prev_column_values[column] * (1.0-alpha));
    prev_column_values[column] = value;

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
        MSGEQ_Bands[band] = max(0, analogRead(MSGEQ0_PIN) - NOISECOMP) * GAIN;
        DEBUG_DO(printBandValue(band, MSGEQ_Bands[band]));

        ++band;
        MSGEQ_Bands[band] = max(0, analogRead(MSGEQ1_PIN) - NOISECOMP) * GAIN;
        DEBUG_DO(printBandValue(band, MSGEQ_Bands[band]));

        digitalWrite(STROBE_PIN, HIGH);
    }
}

bool shouldPrint()
{
    return (0 == (loop_counter % PRINT_INTERVAL));
}

void printBandPrologue()
{
    if (shouldPrint()) {
        DEBUG_DO(Serial.print("\n\n\n\n\n\n"));
        DEBUG_DO(Serial.print("--"));
        DEBUG_DO(Serial.print(loop_counter));
        DEBUG_DO(Serial.print("-- Brightness: "));
        DEBUG_DO(Serial.print(g_curr_brightness));
        DEBUG_DO(Serial.println());
    }
}

void printBandValue(int band, int value)
{
    if (shouldPrint()) {
        DEBUG_DO(Serial.print(band));
        if (band < 10) {
            DEBUG_DO(Serial.print(": "));
        }
        else {
            DEBUG_DO(Serial.print(":"));
        }

        DEBUG_DO(Serial.print(MSGEQ_Bands[band]));
        DEBUG_DO(Serial.println());
    }
}

float mapToFloat(int value, int in_min, int in_max, float out_min, float out_max) {
    return ((float)value - (float)in_min) * ((float)out_max - (float)out_min) / ((float)in_max - (float)in_min) + (float)out_min;
}
