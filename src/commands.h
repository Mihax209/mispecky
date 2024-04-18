#pragma once

enum serial_commands {
    BRIGHTNESS_COMMAND='B',
    EFFECT_COMMAND='E',
    CUSTOM_COLOR_COMMAND='C',
};

void COMMANDS_checkSerialCommands();
