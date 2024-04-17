#pragma once

enum serial_commands {
    BRIGHTNESS_COMMAND='B',
    EFFECT_COMMAND='E',
};

void COMMANDS_checkSerialCommands();
