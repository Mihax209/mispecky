#pragma once

#include <limits.h>

#define DEBUG

/* DEBUG MACROS */
#define PRINT_INTERVAL (60)
extern unsigned long global_loop_counter;

#ifdef DEBUG
#define DEBUG_DO(_x)                                \
({                                                  \
    if (0 == (global_loop_counter % PRINT_INTERVAL)) {   \
        (_x);                                       \
    }                                               \
})
#else
#define DEBUG_DO(_x)
#endif

/* GENERAL MACROS */
#define LENOF(_arr) ((sizeof(_arr)) / (sizeof((_arr)[0])))

/* These definitions don't do nothing, it's just to visually see what we mean in different pin assignments */
#define ANALOG(x)   (x)
#define DIGITAL(x)  (x)

/* PINS */
#define RESET_PIN   DIGITAL(2)      // Pin to instruct MSGEQ7 to return to band zero and inspect it
#define STROBE_PIN  DIGITAL(3)      // Pin to instruct MSGEQ7 IC's to inspect next band (band 0 thru 6)
#define HBEAT_PIN   DIGITAL(4)      // Pin for the hearbteat LED
#define DATA_PIN    DIGITAL(5)      // Pin for serial communication with LED string
#define MSGEQ0_PIN  ANALOG(0)
#define MSGEQ1_PIN  ANALOG(1)
#define BRIGHT_PIN  ANALOG(4)
#define SMOOTH_PIN  ANALOG(2)

/* EQ CONFIG */
#define EQ_BANDS    (14)
#define NOISECOMP   (70)
#define GAIN        (1.2)
#define EQ_DELTA    (5.0)

/* LED CONFIG */
#define ROWS                    (23)
#define COLUMNS                 (12)
#define AMOUNT_TRIMMED          (4)
#define DEFAULT_BRIGHT          (1)
#define MAX_BRIGHTNESS          (35)
#define PEAK_INDICATOR_TIMEOUT  (25)
