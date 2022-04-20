#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "dealerErrors.h"
#include "2310dealer.h"
#include "2310X.h"

/* Global array - Stores PIDs of child processes. */
pid_t* childrenIDs;

/* Global variable - stores length of childrenIDs array. */
int numChildren;

int main(int argc, char** argv) {
    if (argc < MIN_NUM_CMD_LINE_ARGS) {
	return dealer_error_message(DEALER_ARGS);
    }
    FILE* deckFile = fopen(argv[1], "r");
    if (!deckFile) {
	return dealer_error_message(DEALER_DECK);
    }
    size_t deckLength = INITIAL_BUFFER_SIZE;
    char* deck = (char*)malloc(deckLength * sizeof(char));

    // validate deck
    DealerExitCodes deckError = validate_deck(&deck, &deckLength, deckFile);
    if (deckError != DEALER_NORMAL) {
	fclose(deckFile); // validate_deck frees deck in case of error
	return dealer_error_message(deckError);
    }
    FILE* pathFile = fopen(argv[2], "r");
    if (!pathFile) {
	fclose(deckFile);
	free(deck);
	return dealer_error_message(DEALER_PATH);
    }
    size_t pathLength = INITIAL_BUFFER_SIZE;
    char* path = (char*)malloc(pathLength * sizeof(char));
    
    // Used to differentiate who called a function that both the dealer and
    // player can call
    bool playerCalled = false;

    // validate path
    DealerExitCodes pathError = validate_path(&path, &pathLength, pathFile,
	    playerCalled);
    if (pathError != DEALER_NORMAL) {
	fclose(pathFile);
	fclose(deckFile);
	free(path);
	free(deck);
	return dealer_error_message(pathError);
    }
    // First 3 arguments are the dealer program, and the deck and path files
    int playerCount = argc - 3;
    setup_signal_handling(playerCount); // Setup sigaction

    DealerExitCodes gameError = start_game(deck, path, playerCount, argv);
    fclose(pathFile);
    fclose(deckFile);
    free(deck); // path free'd in control_game() (called by start_game())
    free(childrenIDs); // If SIGHUP is not received, free
    return dealer_error_message(gameError);
}

CardType get_card_type(char card) {
    if (card == 'A') {
	return CARD_A;
    }

    if (card == 'B') {
	return CARD_B;
    }
    
    if (card == 'C') {
	return CARD_C;
    }

    if (card == 'D') {
	return CARD_D;
    }

    if (card == 'E') {
	return CARD_E;
    }
    return CARD_ERROR;
}

DealerExitCodes validate_deck(char** deckFromFile, size_t* deckLength,
	FILE* deckFile) {
    if (get_line(deckFromFile, deckLength, deckFile),
	    strlen(*deckFromFile) != 0) {
	char* deckErrors = NULL;
	int numCards = strtol(*deckFromFile, &deckErrors, 10);

	if (numCards < MIN_NUM_CARDS_IN_DECK) {
	    free(*deckFromFile);
	    return DEALER_DECK;
	}

	// Check each card in deck is a valid card
	for (int card = 0; card < strlen(deckErrors); card++) {
	    if (get_card_type(deckErrors[card]) == CARD_ERROR) {
		free(*deckFromFile);
		return DEALER_DECK;
	    }
	}

	// Ensure the number of cards presented in the deck file is accurate
	if (strlen(deckErrors) != numCards) {
	    free(*deckFromFile);
	    return DEALER_DECK;
	}
    } else {
	free(*deckFromFile);
	return DEALER_DECK;
    }
    // Ensure deck only contains one line
    char* checkMoreLines = (char*)malloc(*deckLength * sizeof(char));
    if (get_line(&checkMoreLines, deckLength, deckFile)) {
	free(*deckFromFile);
	free(checkMoreLines);
	return DEALER_DECK;
    }
    free(checkMoreLines);
    return DEALER_NORMAL;
}

DealerExitCodes start_game(char* deck, char* path, int playerCount,
	char** argv) {
    // Initialise dynamic arrays to store the read and write pipes
    FILE** readPipes = (FILE**)malloc(playerCount * sizeof(FILE*));
    FILE** writePipes = (FILE**)malloc(playerCount * sizeof(FILE*));
    for (int player = 0; player < playerCount; player++) {
	int toPlayer[2], fromPlayer[2];
    
	// pipe returns 0 on success - check for failure
	if (pipe(toPlayer) || pipe(fromPlayer)) {
	    free_and_close_pipes(readPipes, writePipes, player);
	    return DEALER_PLAYER;
	}
	pid_t processID = fork();

	// check if fork() call failed
	if (processID < 0) {
	    free_and_close_pipes(readPipes, writePipes, player);
	    return DEALER_PLAYER;
	} else if (!processID) {
	    childrenIDs[player] = getpid(); // store child PID
	    start_players(toPlayer, fromPlayer, argv, playerCount, player);
	}
	// Attempt to close(); close() returns a non-zero int on error - check
	if (close(toPlayer[READ_END]) || close(fromPlayer[WRITE_END])) {
	    free_and_close_pipes(readPipes, writePipes, player);
	    return DEALER_PLAYER;
	}
	readPipes[player] = fdopen(fromPlayer[READ_END], "r");
	writePipes[player] = fdopen(toPlayer[WRITE_END], "w");

	// Check if fdopen failed
	if (!writePipes[player] || !readPipes[player]) {
	    // pass in player - 1 to avoid closing after failed fdopen
	    free_and_close_pipes(readPipes, writePipes, player - 1);
	    return DEALER_PLAYER;
	}

	// Successful starting of players should ensure all players return a ^
    	if (fgetc(readPipes[player]) != '^') {
	    free_and_close_pipes(readPipes, writePipes, player);
	    return DEALER_PLAYER;
	}
    }

    // Start communication with players and play game
    DealerExitCodes gameError = control_game(&readPipes, &writePipes,
	    playerCount, deck, path);
    free_and_close_pipes(readPipes, writePipes, playerCount);
    return gameError;
}

void start_players(int toPlayer[2], int fromPlayer[2], char** argv,
	int playerCount, int player) {
    // close returns 0 on success - check for failure
    if (close(toPlayer[WRITE_END]) || close(fromPlayer[READ_END])) {
	exit(DEALER_PLAYER);
    }

    // Four arguments must be passed into the execvp() call: The player
    // program to start, its command line arguments (player count and ID), and
    // NULL.
    char** playerArgs = (char**)malloc(4 * sizeof(char*));
    for (int i = 0; i < 4; i++) {
	playerArgs[i] = (char*)malloc(INITIAL_BUFFER_SIZE *
		sizeof(char));
    }
    // exclude first 3 args of dealer argv (dealer program, deck, and path)
    playerArgs[0] = argv[player + 3];
    sprintf(playerArgs[1], "%d", playerCount);
    sprintf(playerArgs[2], "%d", player);
    playerArgs[3] = NULL;

    // suppress player's stderr by re-directing to /dev/null
    int devNull = open("/dev/null", O_WRONLY);
    
    // Check if dup2() failed and attempt to close() - check if close() failed
    // as well.
    if (dup2(toPlayer[READ_END], READ_END) == ERROR_RETURN ||
	    dup2(fromPlayer[WRITE_END], WRITE_END) ==
	    ERROR_RETURN || dup2(devNull, ERROR_END) == ERROR_RETURN) {
	if (close(toPlayer[READ_END]) ||
		close(fromPlayer[WRITE_END]) || close(devNull)) {
	    exit(DEALER_PLAYER);
	}
	exit(DEALER_PLAYER);
    }
    if (close(toPlayer[READ_END]) ||
	    close(fromPlayer[WRITE_END]) || close(devNull)) {
	exit(DEALER_PLAYER);
    }
    execvp(playerArgs[0], playerArgs);
    exit(DEALER_PLAYER); // In the case that execvp fails
}

DealerExitCodes control_game(FILE*** readPipes, FILE*** writePipes,
	int playerCount, char* deck, char* path) {
    Game* game = init_game(path, playerCount);
    // Used to differentiate who called a function that both the dealer and
    // player can call
    bool playerCalled = false;

    // send path to all players
    for (int player = 0; player < playerCount; player++) {
	fprintf((*writePipes)[player], "%s\n", path);
	fflush((*writePipes)[player]);
    }
    
    // Start and play game
    display_game(game, playerCalled);
    while (!is_game_over(game)) {
	DealerExitCodes messageError = send_and_receive_messages(game, deck,
		path, readPipes, writePipes, playerCalled);
	if (messageError != DEALER_NORMAL) {
	    return messageError;
	}
    }

    // Notify players of normal game over. Clean up, show scores and finish.
    for (int player = 0; player < playerCount; player++) {
	fprintf((*writePipes)[player], "DONE\n");
	fflush((*writePipes)[player]);
    }
    calculate_final_scores(game, playerCalled);
    free_game(game, path);
    return DEALER_NORMAL;
}

DealerExitCodes send_and_receive_messages(Game* game, char* deck, char* path,
	FILE*** readPipes, FILE*** writePipes, bool playerCalled) {
    // Form string to store DO messages
    size_t doLength = INITIAL_BUFFER_SIZE;
    char* getDo = (char*)malloc(doLength * sizeof(char));
    
    int whoseTurn = calculate_whose_turn(game);
    
    // Ask the player whose turn it is to send back a move
    fprintf((*writePipes)[whoseTurn], "YT\n");
    fflush((*writePipes)[whoseTurn]);
    
    // Attempt to read the player message, check for EOF
    if (get_line(&getDo, &doLength, (*readPipes)[whoseTurn]),
	    strlen(getDo) != 0) {
	// Ensure message received is a valid DO message
	if (get_message_type(game, game->players[whoseTurn], getDo) ==
		MESSAGE_DO) {
	    // First 2 chars are the letters DO, extract the site number.
	    // Message has been validated so error buffer can be NULL.
	    int siteToMoveTo = strtol(getDo + 2, NULL, 10);

	    // Form the required HAP message and send to all players
	    char* hapMessage = create_hap_message(game, whoseTurn,
		    siteToMoveTo, deck);
	    for (int player = 0; player < game->playerCount; player++) {
		fprintf((*writePipes)[player], "%s\n", hapMessage);
		fflush((*writePipes)[player]);
	    }

	    // Update game details and re-display game and player details
	    process_hap_details(game, hapMessage, playerCalled);
	    free(hapMessage);
	    display_game(game, playerCalled);
	} else {
	    // Dealer should only receive (valid) DO messages
	    free(getDo);
	    handle_early_game_over(writePipes, game, path);
	    return DEALER_COMMUNICATION;
	}
    } else {
	// Handle other communication errors (e.g. unexpected EOF on stdin)
	free(getDo);
	handle_early_game_over(writePipes, game, path);
	return DEALER_COMMUNICATION;
    }
    free(getDo);
    return DEALER_NORMAL;
}

void setup_signal_handling(int playerCount) {
    // Set up struct sigaction for handling SIGHUP
    struct sigaction sighupHandlingSetup;

    // Initialise members of struct sigaction to ensure proper memory handling
    memset(&sighupHandlingSetup, 0, sizeof(struct sigaction));

    sighupHandlingSetup.sa_handler = kill_and_reap_children;
    sighupHandlingSetup.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sighupHandlingSetup, NULL);

    // Ensure SIGPIPE does not interfere with sending EARLY messages if a
    // player terminates unexpectedly
    struct sigaction sigpipeHandlingSetup;
    memset(&sigpipeHandlingSetup, 0, sizeof(struct sigaction));
    sigpipeHandlingSetup.sa_handler = SIG_IGN; // Ignore the siganl
    sigpipeHandlingSetup.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sigpipeHandlingSetup, NULL);

    // Setup array to store child PIDs. Require numChildren global as
    // playerCount is required by several functions.
    numChildren = playerCount;
    childrenIDs = (pid_t*)malloc(numChildren * sizeof(pid_t));
}

void kill_and_reap_children(int signal) {
    for (int child = 0; child < numChildren; child++) {
	// SIGKILL cannot be handled. Ensures that any player program run by
	// the dealer is killed and reaped (i.e. removes the concern of player
	// programs having handlers that prevent them from being killed)
	kill(childrenIDs[child], SIGKILL);
	waitpid(childrenIDs[child], NULL, 0);
    }
    exit(DEALER_COMMUNICATION);
}

int calculate_whose_turn(Game* game) {
    // Turn belongs to player furthest back. Iterate from first site onwards
    for (int site = 0; site < game->path->numSites; site++) {
	// Turn belongs to player at bottom of aforementioned site. Iterate
	// from bottom of site upwards
	for (int player = game->playerCount - 1; player >= 0; player--) {
	    // Parts of the site unoccupied have value INVALID_PLAYER_ID. Look
	    // for first part of site which has a player (i.e. bottom player)
	    if (game->path->sites[site].playersAtSite[player] !=
		    INVALID_PLAYER_ID) {
		return game->path->sites[site].playersAtSite[player];
	    }
	}
    }
    return INVALID_PLAYER_ID; // Something went wrong, should never reach here
}

bool is_game_over(Game* game) {
    // The game is over when all players are on the last site. Count how many
    // players are *not* on the last site. If this is 0, the game is over.
    int numAvailableSpacesOnLastSite = 0;
    for (int player = 0; player < game->playerCount; player++) {
	int finalSite = game->path->numSites - 1;
	if (game->path->sites[finalSite].playersAtSite[player] ==
		INVALID_PLAYER_ID) {
	    numAvailableSpacesOnLastSite++;
	}
    }
    // Game is over if numAvailableSpacesOnLastSite == 0
    return (!numAvailableSpacesOnLastSite);
}

char* create_hap_message(Game* game, int movingPlayer, int newSite,
	char* deck) {
    int movingPlayerMoney = game->players[movingPlayer]->money;
    char* hapMessage = (char*)malloc(INITIAL_BUFFER_SIZE * sizeof(char));
    char* newSiteType = game->path->sites[newSite].type;
    int changeInPoints = 0;
    int changeInMoney = 0;
    CardType cardDrawn = CARD_ERROR;

    switch(get_site_type(newSiteType)) {
	case SITE_MO:
	    changeInMoney = 3;
	    break;
	case SITE_DO:
	    changeInMoney -= movingPlayerMoney;
	    changeInPoints = floor(movingPlayerMoney / 2);
	    break;
	case SITE_RI:
	    cardDrawn = draw_next_card(game, deck);
	    break;
	default:
	    // Other card types will not change the above parts of the HAP
	    // message. Their actions are handled in process_hap_details()
	    break;
    }
    sprintf(hapMessage, "HAP%d,%d,%d,%d,%d", movingPlayer, newSite,
	    changeInPoints, changeInMoney, cardDrawn);
    return hapMessage;
}

CardType draw_next_card(Game* game, char* deck) {
    // The deck file is already validated, hence the first non-integer char
    // is the first card to be drawn. Hence, deckWithoutLength gets populated
    // with the correct values
    char* deckWithoutLength = NULL;
    int deckLength = strtol(deck, &deckWithoutLength, 10);
    
    // To draw the next card, we must calculate the current card. Observe that
    // if we reach the end of the deck, we simply go back to the start, and as
    // we are observing the deck via deckWithoutLength, it is clear that the
    // index of the deck that gives the next card to be drawn is given by
    // numCardsAlreadyDrawn % deckLength.
    int numCardsAlreadyDrawn = 0;
    for (int player = 0; player < game->playerCount; player++) {
	for (int cardType = 0; cardType < NUM_CARD_TYPES; cardType++) {
	    // Count the number of cards each player has of each card type
	    numCardsAlreadyDrawn += game->players[player]->numCards[cardType];
	}
    }
    int nextCardIndex = numCardsAlreadyDrawn % deckLength;

    // Return the card type (in HAP format, i.e. A = 1, B = 2, etc.)
    return get_card_type(deckWithoutLength[nextCardIndex]);
}

void handle_early_game_over(FILE*** writePipes, Game* game, char* path) {
    for (int player = 0; player < game->playerCount; player++) {
	fprintf((*writePipes)[player], "EARLY\n");
	fflush((*writePipes)[player]);
    }
    free_game(game, path);
}

void free_and_close_pipes(FILE** readPipes, FILE** writePipes,
	int playerCount) {
    for (int pipe = 0; pipe < playerCount; pipe++) {
	fclose(writePipes[pipe]);
	fclose(readPipes[pipe]);
    }
    free(writePipes);
    free(readPipes);
}
