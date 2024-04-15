#pragma once

enum serial_commands {
    BRIGHTNESS_COMMAND='B'
};

void COMMANDS_checkSerialCommands();
void checkAndUpdateBrightness(String& value_str);
