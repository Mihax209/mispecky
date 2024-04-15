#include <Arduino.h>
#include <FastLED.h>

#include <common.h>
#include <led.h>
#include <commands.h>

void COMMANDS_checkSerialCommands()
{
    if (Serial.available() <= 0) {
        return;
    }

    String input_string = Serial.readStringUntil('\n');
    DEBUG_DO(Serial.print("got input: "));
    DEBUG_DO(Serial.println(input_string));

    if (input_string.charAt(1) != ' ') {
        Serial.println("ERROR: invalid command format");
        return;
    }

    char command_char = input_string.charAt(0);
    String command_data = input_string.substring(2);
    command_data.trim();
    if (command_data.length() == 0) {
        Serial.println("ERROR: no command data received\n");
        return;
    }

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

    if ((new_brightness == 0) && (value > 0)) {
        new_brightness++;
    }

    LED_updateBrightness(new_brightness);
}
