#include <Arduino.h>
#include <FastLED.h>

#include <common.h>
#include <led.h>
#include <commands.h>

#define SUCCESS_PRINT   "ACK"
#define ERROR_PRINT     "ERROR"

void handleBrightnessCommand(String& value_str);
void handleEffectCommand(String& value_str);
void handleCustomColorCommand(String& value_str);

void COMMANDS_checkSerialCommands()
{
    if (Serial.available() <= 0) {
        return;
    }

    String input_string = Serial.readStringUntil('\n');
    DEBUG_DO(Serial.print("got input: "));
    DEBUG_DO(Serial.println(input_string));

    if (input_string.charAt(1) != ' ') {
        Serial.println(ERROR_PRINT": invalid command format");
        return;
    }

    char command_char = input_string.charAt(0);
    String command_data = input_string.substring(2);
    command_data.trim();
    if (command_data.length() == 0) {
        Serial.println(ERROR_PRINT": no command data received");
        return;
    }

    switch (command_char)
    {
    case BRIGHTNESS_COMMAND:
        handleBrightnessCommand(command_data);
        break;
    case EFFECT_COMMAND:
        handleEffectCommand(command_data);
        break;
    case CUSTOM_COLOR_COMMAND:
        handleCustomColorCommand(command_data);
        break;
    default:
        Serial.print("Invalid command: "); Serial.println(command_char);
    }
}

void handleBrightnessCommand(String& value_str)
{
    int value = atoi(value_str.c_str());
    if ((value > 100) || (value < 0)) {
        Serial.print(ERROR_PRINT": brightness should be between 0 and 100 (got ");
        Serial.print(value); Serial.println(")");
        return;
    }

    int new_brightness = (int)map(value, 0, 100, 0, MAX_BRIGHTNESS);

    if ((new_brightness == 0) && (value > 0)) {
        new_brightness++;
    }

    LED_updateBrightness(new_brightness);

    Serial.print(SUCCESS_PRINT": "); Serial.println(value);
}

void handleEffectCommand(String& value_str)
{
    int value = atoi(value_str.c_str());
    if ((value >= EFFECTS_COUNT) || (value < 0)) {
        Serial.print(ERROR_PRINT": effect should be between 0 and");
        Serial.print(EFFECTS_COUNT); Serial.print(" (got ");
        Serial.print(value); Serial.println(")");
        return;
    }

    LED_setColorMatrix((color_effect)value);

    Serial.print(SUCCESS_PRINT": "); Serial.println(value);
}

void handleCustomColorCommand(String& value_str)
{
    if (value_str.length() != 20)
    {
        Serial.print(ERROR_PRINT": bad custom color command. Got: ");
        Serial.println(value_str);
        return;
    }

    unsigned long low,mid,high = 0;
    char *p_end;
    
    low = strtoul(value_str.c_str(), &p_end, HEX);
    mid = strtoul(p_end, &p_end, HEX);
    high = strtoul(p_end, NULL, HEX);

    if (low == 0 || mid == 0 || high == 0)
    {
        Serial.println(ERROR_PRINT": Color conversion failed");
        return;
    }

    LED_setCustomColorMatrix(low, mid, high);

    Serial.print(SUCCESS_PRINT": ");
    Serial.print(low); Serial.print(", ");
    Serial.print(mid); Serial.print(", ");
    Serial.println(high);
}
