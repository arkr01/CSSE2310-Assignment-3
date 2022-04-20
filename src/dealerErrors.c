#include <stdio.h>
#include "dealerErrors.h"

DealerExitCodes dealer_error_message(DealerExitCodes dealerExitType) {
    // The dealer error message to be fprinted to stderr
    const char* dealerErrorMessage = "";

    switch (dealerExitType) {
	case DEALER_NORMAL:
	    return DEALER_NORMAL;
	case DEALER_ARGS:
	    dealerErrorMessage = "Usage: 2310dealer deck path p1 {p2}";
	    break;	
	case DEALER_DECK:
	    dealerErrorMessage = "Error reading deck";
	    break;
	case DEALER_PATH:
	    dealerErrorMessage = "Error reading path";
	    break;
	case DEALER_PLAYER:
	    dealerErrorMessage = "Error starting process";
	    break;
	case DEALER_COMMUNICATION:
	    dealerErrorMessage = "Communications error";
	    break;
    }

    fprintf(stderr, "%s\n", dealerErrorMessage);
    return dealerExitType;
}
