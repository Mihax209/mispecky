#pragma once

enum color_effect {
    INVERTED_COLOR = 0,
    DEFAULT_COLOR,
    VERTICAL_GRADIENT,
    HORITZONTAL_GRADIENT,
    EFFECTS_COUNT,
};


/* LED CONFIG */
#define ROWS                    (23)
#define COLUMNS                 (12)
#define AMOUNT_TRIMMED          (4)
#define DEFAULT_BRIGHT          (1)
#define MAX_BRIGHTNESS          (35)


void LED_init();
void LED_updateBrightness(int brightness);
void LED_updateLEDMatrix();
void LED_setColorMatrix(enum color_effect effect);