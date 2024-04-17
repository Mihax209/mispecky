#pragma once

enum color_effect {
    DEFAULT_COLOR = 0,
    INVERTED_COLOR,
    VERTICAL_GRADIENT,
    HORITZONTAL_GRADIENT,
    EFFECTS_COUNT,
};


void LED_init();
void LED_updateBrightness(int brightness);
void LED_updateLEDMatrix();
void LED_setColorMatrix(enum color_effect effect);