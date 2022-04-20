#ifndef PLAYER_ERRORS_H
#define PLAYER_ERRORS_H

#include <stdio.h>

/* The number of command arguments that all player programs take. */
#define NUM_COMMAND_LINE_ARGS 3

/* Player Exit Codes */
typedef enum {
    PLAYER_NORMAL = 0,
    PLAYER_ARGS = 1,
    PLAYER_COUNT = 2,
    PLAYER_ID = 3,
    PLAYER_PATH = 4,
    PLAYER_EARLY = 5,
    PLAYER_COMMUNICATION = 6
} PlayerExitCodes;

/* Takes in the player exit code. Returns the player exit code and displays
 * the respective player error message. */
PlayerExitCodes player_error_message(PlayerExitCodes playerExitType);

#endif
