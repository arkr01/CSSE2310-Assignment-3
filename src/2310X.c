#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "playerErrors.h"
#include "dealerErrors.h"
#include "2310X.h"

PlayerExitCodes setup_player(int argc, char** argv, Game** game,
	Player** thisPlayer, char** path) {
    if (argc != NUM_COMMAND_LINE_ARGS) {
	return player_error_message(PLAYER_ARGS);
    }

    char* playerCountInput = argv[1];
    char* playerCountErrors = NULL;
    int playerCount = strtol(playerCountInput, &playerCountErrors, 10);
 
    // At least one player must exist
    if (playerCount < 1 || strtol_invalid(playerCountInput,
	    playerCountErrors)) {
	return player_error_message(PLAYER_COUNT);
    }

    char* thisPlayerIDInput = argv[2];
    char* thisPlayerIDErrors = NULL;
    int thisPlayerID = strtol(thisPlayerIDInput, &thisPlayerIDErrors, 10);

    // Player ID must be non-negative and must be less than the player count
    if (thisPlayerID < 0 || thisPlayerID >= playerCount ||
	    strtol_invalid(thisPlayerIDInput, thisPlayerIDErrors)) {
	return player_error_message(PLAYER_ID);
    }

    printf("^");
    fflush(stdout);
 
    size_t pathLength = INITIAL_BUFFER_SIZE;
    *path = (char*)malloc(pathLength * sizeof(char));

    // Used to differentiate who called a function that both the dealer and
    // player can call
    bool playerCalled = true;
   
    PlayerExitCodes pathError = validate_path(path, &pathLength, stdin,
	    playerCalled);

    if (pathError != PLAYER_NORMAL) {
	free(*path);
	return player_error_message(pathError);
    }

    // Set up game data structures
    *game = init_game(*path, playerCount);
    *thisPlayer = (*game)->players[thisPlayerID];
    return PLAYER_NORMAL;
}

Game* init_game(char* pathFromFile, int playerCount) {
    Game* game = (Game*)malloc(sizeof(Game));
    game->playerCount = playerCount;
    init_game_players(game);
    init_game_path(game, pathFromFile);
    init_game_site_players(game);
    return game;
}

void init_game_path(Game* game, char* pathFromFile) {
    Path* path = (Path*)malloc(sizeof(Path));
    
    // pathFromFile already validated, hence no need for error buffer
    path->numSites = strtol(pathFromFile, NULL, 10);
    path->sites = (Site*)malloc(path->numSites * sizeof(Site));

    // Get the path without the site quantity and semi-colon. Path begins with
    // barrier
    char* pathSites = index(pathFromFile, ':');
    int pathIndex = 0;

    for (int site = 0; site < path->numSites; site++) {	
	// Iterate through each char in the site type
	for (int j = 0; j < SITE_LENGTH; j++) {
	    // Assign chars of site type until last char, which should be \0
	    if (j != SITE_LENGTH - 1) {
		path->sites[site].type[j] = pathSites[pathIndex++];
	    } else {
		path->sites[site].type[j] = '\0'; // Site type is string

		// Index indicates this char is site limit. Ensure barrier
		// can hold all players
		if (isdigit(pathSites[pathIndex])) {
		    path->sites[site].limit = atoi(&pathSites[pathIndex++]);
		} else {
		    path->sites[site].limit = game->playerCount;
		    pathIndex++;
		}
	    }
	}
    }
    game->path = path;
}

void init_game_site_players(Game* game) {
    for (int site = 0; site < game->path->numSites; site++) {
	// Initialise dynamic array to store players at site, used to ensure
	// correct ordering when displaying players in order of most recent
	// arrival to a site. Initialise each element to INVALID_PLAYER_ID.
	game->path->sites[site].playersAtSite =
		(int*)malloc(game->playerCount * sizeof(int));

	for (int player = 0; player < game->playerCount; player++) {
	    // At the beginning of the game, all sites but the first should
	    // have no players
	    if (site != 0) {
		game->path->sites[site].playersAtSite[player] =
			INVALID_PLAYER_ID;
	    } else {
		// Ensure descending order of array values but ascending order
		// of array index
		game->path->sites[site].playersAtSite[player] =
			game->playerCount - 1 - player;
	    }
	}
    }
}

void init_game_players(Game* game) {
    game->players = (Player**)malloc(game->playerCount * sizeof(Player*));
    for (int player = 0; player < game->playerCount; player++) {
	game->players[player] = (Player*)malloc(sizeof(Player));
	game->players[player]->playerID = player;
	game->players[player]->money = 7; // Each player starts with 7 money
	game->players[player]->numPoints = 0;
	game->players[player]->currentSite = 0;
	game->players[player]->numV1SitesVisited = 0;
	game->players[player]->numV2SitesVisited = 0;
	game->players[player]->numCards[0] = 0;
	game->players[player]->numCards[1] = 0;
	game->players[player]->numCards[2] = 0;
	game->players[player]->numCards[3] = 0;
	game->players[player]->numCards[4] = 0;
    }
}

bool strtol_invalid(char* input, char* error) {
    // From man strtol: strtol call valid if (*input != '\0' &&
    // *error == '\0')
    if (!(*input != '\0' && *error == '\0')) {
	return true;
    }
    return false;
}

int validate_path(char** pathFromFile, size_t* pathLength, FILE* pathSource,
	bool playerCalled) {
    // Populate *pathFromFile and throw away the result of get_line. Unless
    // nothing was added (i.e. immediate EOF), the contents should be
    // processed, hence the return value of get_line is irrelevant here
    if (get_line(pathFromFile, pathLength, pathSource),
	    strlen(*pathFromFile) != 0) {
	// Only 1 semi-colon should exist in the path (i.e. immediately
	// following the number of sites). No whitespace should exist.
	if (character_counter(*pathFromFile, ';') != 1 ||
		character_counter(*pathFromFile, ' ') != 0 ||
		character_counter(*pathFromFile, '\t') != 0) {
	    return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	}
	char* pathErrors = NULL;
	int numSites = strtol(*pathFromFile, &pathErrors, 10);
	
	// After extracting the number of sites from the path, the semi-colon
	// should be the first character in the pathErrors string.
	if (numSites < MIN_SITES || pathErrors[0] != ';') {
	    return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	}
	if (validate_path_sites(pathErrors, playerCalled) == PLAYER_PATH ||
		validate_path_sites(pathErrors, playerCalled) ==
		DEALER_PATH) {
	    return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	}

	// Validate numSites. Note that apart from accounting for the length
	// of each site, we must add 1 char for the semi-colon after the
	// number of sites
	if (strlen(pathErrors) != (numSites * SITE_LENGTH + 1)) {
	    return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	}
    } else {
	// If *nothing* but EOF is detected, then return communications error
	return (playerCalled) ? PLAYER_COMMUNICATION : DEALER_COMMUNICATION;
    }
    // The dealer must accept a path file consisting of a single line. Prevent
    // extra lines from path file format. The player does not take the file
    // itself, but may be sent a file containing the path and some messages
    if (!playerCalled) {
	size_t extraLinesLength = INITIAL_BUFFER_SIZE;
	char* testExtraLines = (char*)malloc(extraLinesLength * sizeof(char));
	if (get_line(&testExtraLines, &extraLinesLength, pathSource)) {
	    free(testExtraLines);
	    return DEALER_PATH;
	}
	free(testExtraLines);
    }
    return (playerCalled) ? PLAYER_NORMAL : DEALER_NORMAL;
}

bool get_line(char** buffer, size_t* lineLength, FILE* sourceOfLine) {
    size_t bufferIndex = 0;
    (*buffer)[0] = '\0';
    int input;
    while (input = fgetc(sourceOfLine), input != EOF && input != '\n') {
	// Check if buffer needs to be expanded (ensure space is available
	// prior to buffer expansion by checking *lineLength - 2)
        if (bufferIndex > *lineLength - 2) {
	    // Create a new, larger buffer and point the current buffer to the
	    // new buffer
            size_t newLineLength = (size_t) (*lineLength * RESIZING_FACTOR);
            void* newBuffer = realloc(*buffer, newLineLength);
            if (newBuffer == 0) { // realloc returns 0 on error
                return false;
            }
            *lineLength = newLineLength;
            *buffer = newBuffer;
        }
        (*buffer)[bufferIndex] = (char)input;
        (*buffer)[++bufferIndex] = '\0'; // Ensure string is null-terminated
    }
    return input != EOF;
}

MessageType get_message_type(Game* game, Player* thisPlayer,
	char* dealerOrPlayerMessage) {
    if (!strcmp(dealerOrPlayerMessage, "YT")) {
	return MESSAGE_YT;
    } 

    if (do_message_valid(game, thisPlayer, dealerOrPlayerMessage)) {
	return MESSAGE_DO;
    }

    if (!strcmp(dealerOrPlayerMessage, "EARLY")) {
	return MESSAGE_EARLY;
    }

    if (!strcmp(dealerOrPlayerMessage, "DONE")) {
	return MESSAGE_DONE;
    }

    if (hap_message_valid(game, dealerOrPlayerMessage)) {
	return MESSAGE_HAP;
    }
    return MESSAGE_ERROR;
}

SiteType get_site_type(char* site) {
    if (!strcmp(site, "Mo")) {
	return SITE_MO;
    }

    if (!strcmp(site, "V1")) {
	return SITE_V1;
    }

    if (!strcmp(site, "V2")) {
	return SITE_V2;
    }

    if (!strcmp(site, "Do")) {
	return SITE_DO;
    }

    if (!strcmp(site, "Ri")) {
	return SITE_RI;
    }

    if (!strcmp(site, "::")) {
	return SITE_BARRIER;
    }
    return SITE_ERROR;
}

int get_first_site_of_type(SiteType siteType, Player* thisPlayer,
	Game* game) {
    int nextSite = thisPlayer->currentSite + 1;
    for (int site = nextSite; site < game->path->numSites; site++) {
	if (!check_site_full(game, site) &&
		!check_barrier_skipped(game, thisPlayer, site) &&
		get_site_type(game->path->sites[site].type) == siteType) {
	    return site;
	}
    }
    return INVALID_SITE;
}

bool do_message_valid(Game* game, Player* thisPlayer,
	char* dealerOrPlayerMessage) { 
    // Here, strstr returns pointer to first occurrence of string "DO" in the
    // dealer/player message. Check if said message begins with "DO"
    if (strstr(dealerOrPlayerMessage, "DO") - dealerOrPlayerMessage != 0) {
	return false;
    }

    // Get the new site from the DOn message (+ 2 to exclude the first 2 chars
    // (i.e. DO)
    char* newSiteErrors = NULL;
    int newSite = strtol(dealerOrPlayerMessage + 2, &newSiteErrors, 10);

    // Check if strtol found any invalid chars in n.
    if (strtol_invalid(dealerOrPlayerMessage + 2, newSiteErrors)) {
	return false;
    }
    
    // Ensure n refers to a real site that is in front of the player.
    if (newSite <= thisPlayer->currentSite ||
	    newSite >= game->path->numSites) {
	return false;
    }
    
    // Ensure the site the player wants to move to is not full. Also ensure
    // the player's move does not skip any barriers
    if (check_site_full(game, newSite) ||
	    check_barrier_skipped(game, thisPlayer, newSite)) {
	return false;
    }
    return true;
}

bool hap_message_valid(Game* game, char* dealerOrPlayerMessage) {
    // Here, strstr returns pointer to first occurrence of string "HAP" in the
    // dealer/player message. Check if said message begins with "HAP"
    if (strstr(dealerOrPlayerMessage, "HAP") - dealerOrPlayerMessage != 0) {
	return false;
    }

    char* errorBuffer = NULL;
    // HAP is 3 chars, so start after HAP, i.e. at index 3
    int playerID = strtol(dealerOrPlayerMessage + 3, &errorBuffer, 10);
    if (hap_message_number_invalid(game, playerID, playerID,
	    MOVE_PLAYER_ID) ||
	    hap_invalid_chars(dealerOrPlayerMessage + 3, errorBuffer)) {
	return false;
    }
    // Iterate through components of HAP message. First component (i.e. Player
    // ID) validation handled, so start at i = MOVE_NEW_SITE. Iterate until
    // i = MOVE_MONEY_CHANGE. Handle final case (i.e. MOVE_CARD_DRAWN)
    // separately.
    char* restOfHap;
    for (int i = MOVE_NEW_SITE; i < MOVE_CARD_DRAWN; i++) {
	// First char of error buffer is a comma, so to obtain the next
	// number to validate, we must check 1 char after the comma.
	restOfHap = errorBuffer + 1;
	int valueToCheck = strtol(restOfHap, &errorBuffer, 10);
	if (hap_invalid_chars(restOfHap, errorBuffer) ||
		hap_message_number_invalid(game, playerID, valueToCheck,
		i)) {
	    return false;
	}
    }
    
    // Ensure card drawn refers to a real card. Also ensure no invalid chars
    // are present.
    restOfHap = errorBuffer + 1;
    int cardDrawn = strtol(restOfHap, &errorBuffer, 10);
    if (strtol_invalid(restOfHap, errorBuffer) ||
	    hap_message_number_invalid(game, playerID, cardDrawn,
	    MOVE_CARD_DRAWN)) {
	return false;
    }
    return true;
}

bool hap_message_number_invalid(Game* game, int playerID, int valueToCheck,
	HapComponent valueType) {
    bool returnValue = true;
    switch(valueType) {
	case MOVE_PLAYER_ID:
	    // Ensure player exists
	    if (valueToCheck >= 0 && valueToCheck < game->playerCount) {
		returnValue = false;
	    }
	    break;
	case MOVE_NEW_SITE:
	    // Ensure newSite is a move forward and refers to a real site.
	    // Also ensure the site the player wants to move to is not full.
	    // Also ensure the player's move does not skip any barriers.
	    if (valueToCheck > game->players[playerID]->currentSite &&
		    valueToCheck < game->path->numSites &&
		    !check_barrier_skipped(game, game->players[playerID],
		    valueToCheck) && !check_site_full(game, valueToCheck)) {
		returnValue = false;
	    }
	    break;
	case MOVE_ADDITIONAL_POINTS:
	    // Ensure additional points is non-negative
	    if (valueToCheck >= 0) {
		returnValue = false;
	    }
	    break;
	case MOVE_MONEY_CHANGE:
	    // strtol returns 0 even on invalid call, and this is already
	    // validated prior to calling this function, hence nothing is to
	    // be checked as MOVE_MONEY_CHANGE can take any integer value
	    returnValue = false;
	    break;
	case MOVE_CARD_DRAWN:
	    if (valueToCheck >= 0 && valueToCheck <= NUM_CARD_TYPES) {
		returnValue = false;
	    }
	    break;
    }
    return returnValue;
}

bool hap_invalid_chars(char* remainderOfHap, char* errorBuffer) {
    // If invalid chars are present, either the first char in the error buffer
    // would not be a comma, or the first char in the remainder of the HAP
    // message would not be a digit.
    if (errorBuffer[0] != ',' || !isdigit(remainderOfHap[0])) {
	// Above condition handles all cases but negative numbers. Ensure that
	// negative numbers are considered valid chars (other functions handle
	// whether negative numbers are allowed in a specific context).
	// NOTE: strtol returns 0 on error so this does not compromise other
	// situations.
	if (strtol(remainderOfHap, NULL, 10) >= 0) {
	    return true;
	}
    }
    return false;
}

void process_hap_details(Game* game, char* hapMessage, bool playerCalled) {
    char* errorBuffer = NULL;
    // HAP is 3 chars, so start after HAP, i.e. at index 3
    int playerID = strtol(hapMessage + 3, &errorBuffer, 10);

    // Site player is moving from
    int originalSite = game->players[playerID]->currentSite;

    // The first element in the error buffer is a comma, hence to obtain the
    // next number, call strtol on errorBuffer + 1
    int newSite = strtol(errorBuffer + 1, &errorBuffer, 10);
    game->players[playerID]->currentSite = newSite;

    // Update site information regarding players at sites
    update_player_sites(game, game->players[playerID], originalSite, newSite); 

    // Update number of V1/V2 sites visted by moving player 
    if (!strcmp(game->path->sites[newSite].type, "V1")) {
	(game->players[playerID]->numV1SitesVisited)++;
    }
    if (!strcmp(game->path->sites[newSite].type, "V2")) {
	(game->players[playerID]->numV2SitesVisited)++;
    }

    game->players[playerID]->numPoints += strtol(errorBuffer + 1,
	    &errorBuffer, 10);

    game->players[playerID]->money += strtol(errorBuffer + 1,
	    &errorBuffer, 10);

    int cardDrawn = strtol(errorBuffer + 1, NULL, 10);
    
    // if a card is actually drawn, i.e. cardDrawn != 0, update the number of
    // cards of the given type that the player has
    if (cardDrawn) {
	// Zero-based indexing means we must subtract 1
	(game->players[playerID]->numCards[cardDrawn - 1])++;
    }
    FILE* output = (playerCalled) ? stderr : stdout;
    display_player_details(game, game->players[playerID], output);
}

void update_player_sites(Game* game, Player* movingPlayer, int originalSite,
	int newSite) {
    // Remove player from original site
    for (int player = 0; player < game->playerCount; player++) {
	if (game->path->sites[originalSite].playersAtSite[player] ==
		movingPlayer->playerID) {
	    game->path->sites[originalSite].playersAtSite[player] =
		    INVALID_PLAYER_ID;
	}
    }

    // Update playersAtSite. Add new player to the first available space
    // (i.e. space not currently occupied by a player), used to maintain
    // correct order of players in game display, i.e. in order of most recent
    // arrival to site
    for (int player = 0; player < game->playerCount; player++) {
	if (game->path->sites[newSite].playersAtSite[player] ==
		INVALID_PLAYER_ID) {
	    game->path->sites[newSite].playersAtSite[player] =
		    movingPlayer->playerID;
	    break; // Ensure to only add player to first available site
	}
    }
}

void display_player_details(Game* game, Player* thisPlayer,
	FILE* displayLocation) {
    fprintf(displayLocation, "Player %d ", thisPlayer->playerID);
    fflush(displayLocation);

    fprintf(displayLocation, "Money=%d ", thisPlayer->money);
    fflush(displayLocation);
    
    fprintf(displayLocation, "V1=%d ", thisPlayer->numV1SitesVisited);
    fflush(displayLocation);
    
    fprintf(displayLocation, "V2=%d ", thisPlayer->numV2SitesVisited);
    fflush(displayLocation);
    
    fprintf(displayLocation, "Points=%d ", thisPlayer->numPoints);
    fflush(displayLocation);
    
    fprintf(displayLocation, "A=%d ", thisPlayer->numCards[0]);
    fflush(displayLocation);
    
    fprintf(displayLocation, "B=%d ", thisPlayer->numCards[1]);
    fflush(displayLocation);
    
    fprintf(displayLocation, "C=%d ", thisPlayer->numCards[2]);
    fflush(displayLocation);
    
    fprintf(displayLocation, "D=%d ", thisPlayer->numCards[3]);
    fflush(displayLocation);
    
    fprintf(displayLocation, "E=%d\n", thisPlayer->numCards[4]);
    fflush(displayLocation);
}

bool check_site_full(Game* game, int move) {
    int numPlayersAtSite = 0;

    // Count the number of players at the site this player wants to move to
    for (int player = 0; player < game->playerCount; player++) {
	if (game->players[player]->currentSite == move) {
	    numPlayersAtSite++;
	}
    }

    // Calculate how many players can move to this site. Check if this is a
    // positive value (i.e. site is not full).
    if (game->path->sites[move].limit - numPlayersAtSite > 0) {
	return false;
    }
    return true;
}

bool check_barrier_skipped(Game* game, Player* movingPlayer, int move) {
    // Iterate through all sites
    for (int site = 0; site < game->path->numSites; site++) {
	// Check if site is barrier and if said barrier is between the player
	// and the player's move
	if (!strcmp(game->path->sites[site].type, "::") && site < move &&
		movingPlayer->currentSite < site) {
	    return true;
	}
    }
    return false;
}

int character_counter(char* stringToSearch, char characterToCount) {
    int characterCount = 0;
    for (int i = 0; stringToSearch[i] != '\0'; i++) {
	if (stringToSearch[i] == characterToCount) {
	    characterCount++;
	}
    }
    return characterCount;
}

char* reverse_string(char* toReverse, size_t toReverseLength) {
    char* reversed = (char*)malloc((toReverseLength + 1) * sizeof(char));
   
    int i = 0;
    for (; i < toReverseLength; i++) {
	reversed[i] = toReverse[toReverseLength - i - 1];
    }
    reversed[i] = '\0';

    return reversed;
}

int validate_path_sites(char* pathSitesFromFile, bool playerCalled) {
    char* reversedPath = reverse_string(pathSitesFromFile,
	    strlen(pathSitesFromFile));

    // Ensure path starts/ends with barrier (checking path ends with
    // barrier is the same as checking reversed path starts with barrier)
    // strstr returns pointer to first index which has specified string (in
    // this case, the barrier). Therefore, the difference should be 1 if the
    // second char (i.e. after the semi-colon) is the start of a barrier, and
    // should be 0 if the last char is the end of a barrier
    if (strstr(pathSitesFromFile, "::-") - pathSitesFromFile != 1 ||
	    strstr(reversedPath, "-::") - reversedPath != 0) {
	free(reversedPath);
	return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
    }

    // Pass in the path sites (+ 1 to exclude the semi-colon)
    if (validate_site_types(pathSitesFromFile + 1, playerCalled) !=
	    PLAYER_NORMAL ||
	    validate_site_player_limits(pathSitesFromFile + 1,
	    playerCalled) != PLAYER_NORMAL ||
	    validate_site_types(pathSitesFromFile + 1, playerCalled) !=
	    DEALER_NORMAL ||
	    validate_site_player_limits(pathSitesFromFile + 1,
	    playerCalled) != DEALER_NORMAL) {
	free(reversedPath);
	return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
    }
    free(reversedPath);
    return (playerCalled) ? PLAYER_NORMAL : DEALER_NORMAL;
}

int validate_site_types(char* pathSitesFromFile, bool playerCalled) {
    // Iterate through each site. The situation where sites are of invalid
    // length is handled within the loop, as this would cascade over to the
    // next iteration
    for (int site = 0; site < strlen(pathSitesFromFile);
	    site += SITE_LENGTH) {
	// Extract each site type (each site is represented by 2 chars)
	char siteTypeToCheck[SITE_LENGTH];
	siteTypeToCheck[0] = pathSitesFromFile[site];
	siteTypeToCheck[1] = pathSitesFromFile[site + 1];
	siteTypeToCheck[2] = '\0';

	// Check if siteTypeToCheck is invalid
	if (get_site_type(siteTypeToCheck) == SITE_ERROR) {
	    return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	}
    }
    return (playerCalled) ? PLAYER_NORMAL : DEALER_NORMAL;
}

int validate_site_player_limits(char* pathSitesFromFile, bool playerCalled) {
    for (int site = 0; site < strlen(pathSitesFromFile);
	    site += SITE_LENGTH) {
	// Extract each site (each site is represented with 2 chars for the
	// site type and 1 char for the site playerLimit)
	char siteWithCapacityToCheck[SITE_LENGTH];
	siteWithCapacityToCheck[0] = pathSitesFromFile[site];
	siteWithCapacityToCheck[1] = pathSitesFromFile[site + 1];
	siteWithCapacityToCheck[2] = pathSitesFromFile[site + 2];

	char playerLimitOfSiteToCheck = siteWithCapacityToCheck[2];

	// Validate barrier site playerLimit
	if (siteWithCapacityToCheck[0] == ':' &&
		siteWithCapacityToCheck[1] == ':') {
	    if (playerLimitOfSiteToCheck == '-') {
		continue;
	    } else {
		return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	    }
	}

	// Ensure player limit is a single, non-zero digit (i.e. a number
	// between 1 and 9)
	if (!isdigit(playerLimitOfSiteToCheck) ||
		playerLimitOfSiteToCheck == '0') {
	    return (playerCalled) ? PLAYER_PATH : DEALER_PATH;
	}
    }

    return (playerCalled) ? PLAYER_NORMAL : DEALER_NORMAL;
}

PlayerExitCodes play_game(Game* game, Player* thisPlayer,
	int (*moveStrategy)(Game* game, Player* thisPlayer)) {
    // Some of the functions called in this function can be called by the
    // dealer. This function can only be called by the player.
    bool playerCalled = true;

    bool gameOver = false; // Becomes true of DONE message is received

    display_game(game, playerCalled);
    while(!gameOver) {
	size_t dealerMessageMax = INITIAL_BUFFER_SIZE;
	char* dealerMessage = (char*)malloc(dealerMessageMax * sizeof(char));

        // Populate dealerMessage and throw away the result of get_line.
	// Unless nothing was added (i.e. immediate EOF), the contents should
	// be processed, hence the return value of get_line is irrelevant here
	if (get_line(&dealerMessage, &dealerMessageMax, stdin),
		strlen(dealerMessage) != 0) {
	    switch(get_message_type(game, thisPlayer, dealerMessage)) {
		case MESSAGE_YT:
		    printf("DO%d\n", moveStrategy(game, thisPlayer));
		    fflush(stdout);
		    free(dealerMessage);
		    break;
		case MESSAGE_DO:
		    // Players should not receive DO messages
		    free(dealerMessage);
		    return PLAYER_COMMUNICATION;
		case MESSAGE_EARLY:
		    free(dealerMessage);
		    return PLAYER_EARLY;
		case MESSAGE_DONE:
		    gameOver = true;
		    free(dealerMessage);
		    calculate_final_scores(game, playerCalled);
		    break;
		case MESSAGE_HAP:
		    process_hap_details(game, dealerMessage, playerCalled);
		    free(dealerMessage);
		    display_game(game, playerCalled);
		    break;
		case MESSAGE_ERROR:
		    free(dealerMessage);
		    return PLAYER_COMMUNICATION;
	    }
	} else {
	    free(dealerMessage);
	    return PLAYER_COMMUNICATION;
	}
    }
    return PLAYER_NORMAL;
}

void calculate_final_scores(Game* game, bool playerCalled) {
    FILE* output = (playerCalled) ? stderr : stdout;

    fprintf(output, "Scores: ");
    fflush(output);
    for (int player = 0; player < game->playerCount; player++) {
	game->players[player]->numPoints +=
		game->players[player]->numV1SitesVisited;
	game->players[player]->numPoints +=
		game->players[player]->numV2SitesVisited;
	calculate_score_from_cards(game->players[player]);
	fprintf(output, "%d", game->players[player]->numPoints);
	fflush(output);

	// Ensure that comma is not printed after last element
	if (player != game->playerCount - 1) {
	    fprintf(output, ",");
	    fflush(output);
	}
    }
    fprintf(output, "\n");
    fflush(output);
}

void calculate_score_from_cards(Player* player) {
    int numCardSets = 0;

    // Non-zero amounts of multiple card types indicate a set of cards.
    // Identify the largest set.
    for (int cardType = 0; cardType < NUM_CARD_TYPES; cardType++) {
	if (player->numCards[cardType] > 0) {
	    numCardSets++;
	}
    }

    // Ensure base case handles when numCardSets == 1 and when
    // numCardSets == 0
    if (numCardSets < 2) {
	for (int cardType = 0; cardType < NUM_CARD_TYPES; cardType++) {
	    player->numPoints += player->numCards[cardType];
	}
    } else {
	if (numCardSets == NUM_CARD_TYPES) {
	    // A full set of all types of cards is worth 10 points
	    player->numPoints += 10;
	} else {
	    // Referencing the assignment spec, it may be observed that the
	    // number of points obtained from each set (excluding the full
	    // set) is given by the below formula (e.g. a set of size 3 gives
	    // 5 points)
	    player->numPoints += 2 * numCardSets - 1;
	}

	// After adding the points obtained from the largest set, remove this
	// set and examine the remaining cards that the player has.
	for (int cardType = 0; cardType < NUM_CARD_TYPES; cardType++) {
	    if (player->numCards[cardType] != 0) {
		(player->numCards[cardType])--;
	    }
	}
	calculate_score_from_cards(player);
    }
}

void display_game(Game* game, bool playerCalled) {
    FILE* output = (playerCalled) ? stderr : stdout;

    // display path
    for (int site = 0; site < game->path->numSites; site++) {
	fprintf(output, "%s ", game->path->sites[site].type);
	fflush(output);
    }
    fprintf(output, "\n");
    fflush(output);

    // Store player positions into 2D array
    int** playerDisplay = init_player_positions(game); 

    // Sort 2D array to fit required game display formatting (i.e. no blank
    // spaces above/below)
    sort_player_positions(game, &playerDisplay);
    
    // With this array construction, game->playerCount rows will be displayed
    // after every move. In the cases where this is unwanted (e.g. all players
    // are on different sites and hence only one row should be displayed), a
    // series of blank rows (i.e. rows with all spaces) will be printed.
    // Ensure this doesn't occur.
    clean_player_display(game, &playerDisplay); 

    // display player positions
    for (int row = 0; row < game->playerCount; row++) {
	// Ensure not to print blank lines full of spaces
	if (playerDisplay[row][0] == EMPTY_LINE) {
	    continue;
	}
	for (int col = 0; col < SITE_LENGTH * game->path->numSites; col++) {
	    if (playerDisplay[row][col] == INVALID_PLAYER_ID) {
		// Ensure correct spacing of players
		fprintf(output, " ");
		fflush(output);
	    } else {
		// Display player
		fprintf(output, "%d", playerDisplay[row][col]);
		fflush(output);
	    }
	}
	fprintf(output, "\n");
	fflush(output);
    }
    // Free playerDisplay
    for (int row = 0; row < game->playerCount; row++) {
	free(playerDisplay[row]);
    }
    free(playerDisplay);
}

int** init_player_positions(Game* game) {
    // Create 2D array to store player positions on game display
    int** playerDisplay = (int**)malloc(game->playerCount * sizeof(int*));
    for (int row = 0; row < game->playerCount; row++) {
	// Allocate enough size to store the path length
	playerDisplay[row] = (int*)malloc(
		(SITE_LENGTH * game->path->numSites) * sizeof(int));
    }

    for (int col = 0; col < SITE_LENGTH * game->path->numSites; col++) {
	for (int player = 0; player < game->playerCount; player++) {
	    if (!(col % SITE_LENGTH)) {
		int currentSite = col / SITE_LENGTH;
		playerDisplay[player][col] =
			game->path->sites[currentSite].playersAtSite[player];
	    } else {
		playerDisplay[player][col] = INVALID_PLAYER_ID;
	    }
	}
    }
    return playerDisplay;
}

void sort_player_positions(Game* game, int*** playerDisplay) {
    for (int col = 0; col < SITE_LENGTH * game->path->numSites; col++) {
	for (int row = 0; row < game->playerCount; row++) {
	    // Compare each element in column with top row and work downwards
	    for (int rowNext = row + 1; rowNext < game->playerCount;
		    rowNext++) {
		
		// Standard swap with temporary variable
		if ((*playerDisplay)[row][col] == INVALID_PLAYER_ID) {
		    int tmp = (*playerDisplay)[row][col];
		    (*playerDisplay)[row][col] =
			    (*playerDisplay)[rowNext][col];
		    (*playerDisplay)[rowNext][col] = tmp;
		}
	    }
	}
    }
}

void clean_player_display(Game* game, int*** playerDisplay) {
    for (int row = 0; row < game->playerCount; row++) {
	int spaceCounter = 0;
	for (int col = 0; col < SITE_LENGTH * game->path->numSites; col++) {
	    if ((*playerDisplay)[row][col] == INVALID_PLAYER_ID) {
		spaceCounter++;
	    }
	}
	// Check if entire row in player display is nothing but spaces (i.e.
	// no players exist on this row). If so, mark the beginning of the row
	// with a sentinel to ensure random extra vertical whitespace is not
	// displayed to user
	if (spaceCounter == SITE_LENGTH * game->path->numSites) {
	    (*playerDisplay)[row][0] = EMPTY_LINE;
	}
    }
}

void free_game(Game* game, char* pathFromFile) {
    // Free the players at the path sites
    for (int site = 0; site < game->path->numSites; site++) {
	free(game->path->sites[site].playersAtSite);
    } 

    // Free the path sites and the path
    free(game->path->sites);
    free(game->path);

    // Free the (validated) path from the given path file
    free(pathFromFile);

    // Free the player representations
    for (int player = 0; player < game->playerCount; player++) {
	free(game->players[player]);
    }
    free(game->players);
    
    // Free the game
    free(game);
}
