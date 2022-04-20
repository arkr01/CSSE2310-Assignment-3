#include <stdio.h>
#include "playerErrors.h"

PlayerExitCodes player_error_message(PlayerExitCodes playerExitType) {
    // The player error message to be fprinted to stderr
    const char* playerErrorMessage = "";

    switch (playerExitType) {
	case PLAYER_NORMAL:
	    return PLAYER_NORMAL;
	case PLAYER_ARGS:
	    playerErrorMessage = "Usage: player pcount ID";
	    break;
	case PLAYER_COUNT:
	    playerErrorMessage = "Invalid player count";
	    break;
	case PLAYER_ID:
	    playerErrorMessage = "Invalid ID";
	    break;
	case PLAYER_PATH:
	    playerErrorMessage = "Invalid path";
	    break;
	case PLAYER_EARLY:
	    playerErrorMessage = "Early game over";
	    break;
	case PLAYER_COMMUNICATION:
	    playerErrorMessage = "Communications error";
	    break;
    }

    fprintf(stderr, "%s\n", playerErrorMessage);
    return playerExitType;
}
