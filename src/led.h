#pragma once

enum color_effect {
    DEFAULT_COLOR = 0,
    DEFAULT2_COLOR,
    CALM_COLOR,
    VERTICAL_GRADIENT,
    HORITZONTAL_GRADIENT,
    EFFECTS_COUNT,
};


/* LED CONFIG */
#define ROWS                    (23)
#define COLUMNS                 (12)
#define AMOUNT_TRIMMED          (4)
#define DEFAULT_BRIGHT          (1)
#define MAX_BRIGHTNESS          (80)


void LED_init();
void LED_updateBrightness(int brightness);
void LED_updateLEDMatrix();
void LED_setColorMatrix(enum color_effect effect);
void LED_setCustomColorMatrix(unsigned long low_color, unsigned long mid_color, unsigned long high_color);