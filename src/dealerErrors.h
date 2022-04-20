#ifndef DEALER_ERRORS_H
#define DEALER_ERRORS_H

#include <stdio.h>

/* Minimum number of command line args for the dealer */
#define MIN_NUM_CMD_LINE_ARGS 4

/* Dealer Exit Codes */
typedef enum {
    DEALER_NORMAL = 0,
    DEALER_ARGS = 1,
    DEALER_DECK = 2,
    DEALER_PATH = 3,
    DEALER_PLAYER = 4,
    DEALER_COMMUNICATION = 5
} DealerExitCodes;

/* Takes in the dealer exit code. Returns the dealer exit code and displays
 * the respective dealer error message. */
DealerExitCodes dealer_error_message(DealerExitCodes dealerExitType);

#endif
