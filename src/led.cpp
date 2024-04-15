#include <Arduino.h>
#include <FastLED.h>

#include <common.h>
#include <spectrum.h>
#include <led.h>

/* PRIVATE FUNCTIONS */
int getColumnHeight(int column);
void printColumnHeight(int col, int height);
void defaultColor();
void invertedColor();
void verticalGradientColor();
void horizontalGradientColor();
float mapToFloat(int value, int in_min, int in_max, float out_min, float out_max);


/* GLOBAL VARIABLES */
float prev_column_values[COLUMNS] = {0};
bool g_peak_indicators_enabled = false;
bool lit_matrix[COLUMNS][ROWS];
uint8_t peak_indicators[COLUMNS] = {0};
unsigned int peak_indicators_timeout[COLUMNS] = {0};

struct CRGB color_matrix[COLUMNS][ROWS];
struct CRGB fastLED_matrix[(COLUMNS*ROWS) - (2*AMOUNT_TRIMMED)];

static int g_curr_brightness = DEFAULT_BRIGHT;


void LED_init()
{
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(fastLED_matrix, LENOF(fastLED_matrix));
    LED_updateBrightness(DEFAULT_BRIGHT);
}


void LED_updateBrightness(int brightness)
{
    g_curr_brightness = brightness;
    FastLED.setBrightness(brightness);
    DEBUG_DO(Serial.print("Brightness updated to: "));
    DEBUG_DO(Serial.println(brightness));
}


void LED_updateLEDMatrix()
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

        DEBUG_DO(printColumnHeight(col, height));
    }

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
    current = SPECTRUM_getBandValue(column + 1);
    if (column == 0) {
        // Bass average with invisible band
        current = max(current, SPECTRUM_getBandValue(0));
    }
    current /= EQ_DELTA;

    value = (current * alpha) + (previous * (1.0-alpha));
    prev_column_values[column] = value;

    return min(ROWS, (int)value);
}


void printColumnHeight(int col, int height)
{
    Serial.print("Column height: "); Serial.print(height);
    Serial.print(". Peak: "); Serial.print(peak_indicators[col]);
    Serial.print(" (T-"); Serial.print(peak_indicators_timeout[col]); Serial.println(")");
}


void LED_setColorMatrix(enum color_effect effect)
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
        DEBUG_DO(Serial.print("ERROR: got to updateColorMatrix with effect num: "));
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

float mapToFloat(int value, int in_min, int in_max, float out_min, float out_max) {
    return ((float)value - (float)in_min) * ((float)out_max - (float)out_min) / ((float)in_max - (float)in_min) + (float)out_min;
}
