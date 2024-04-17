#pragma once

enum color_effect {
    INVERTED_COLOR = 0,
    DEFAULT_COLOR,
    VERTICAL_GRADIENT,
    HORITZONTAL_GRADIENT,
    EFFECTS_COUNT,
};


void LED_init();
void LED_updateBrightness(int brightness);
void LED_updateLEDMatrix();
void LED_setColorMatrix(enum color_effect effect);