#ifndef DEALER_H
#define DEALER_H

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
#include "2310X.h"

/* As per the assignment spec, the minimum number of cards allowed in a deck
 * file is 4. */
#define MIN_NUM_CARDS_IN_DECK 4

/* Denotes file descriptor of read end (usually stdin). */
#define READ_END 0

/* Denotes file descriptor of write end (usually stdout). */
#define WRITE_END 1

/* Denotes file descriptor of error end (usually stderr). */
#define ERROR_END 2

/* Several system calls return -1 on error. Check for this. */
#define ERROR_RETURN (-1)

/* Card Types */
typedef enum {
    CARD_ERROR = 0,
    CARD_A = 1,
    CARD_B = 2,
    CARD_C = 3,
    CARD_D = 4,
    CARD_E = 5
} CardType;

/* Takes in the deck representation of a card and returns the appropriate card
 * type. */
CardType get_card_type(char card);

/* Takes in the deck file, as well as an empty buffer to store the deck, and
 * its max size. Validates the deck file contents and returns the appropriate
 * dealer exit code. Does not handle any file work. */
DealerExitCodes validate_deck(char** deckFromFile, size_t* deckLength,
	FILE* deckFile);

/* Takes in the validated deck and path, the number of players, as well as the
 * command-line arguments (to extract the player programs). Entry point for
 * game. Returns the appropriate dealer exit code. */
DealerExitCodes start_game(char* deck, char* path, int playerCount,
	char** argv);

/* Takes in the (piped) file descriptors, the command-line arguments, the
 * player count, and the current player ID. Ensures valid start of player
 * processes. */
void start_players(int toPlayer[2], int fromPlayer[2], char** argv,
	int playerCount, int player);

/* Takes in the collection of read and write pipes to communicate with the
 * players, as well as the number of players, and the (validated) deck and
 * path. Controls main gameplay and communcation between players. Returns the
 * appropriate exit code at the end of the game. */
DealerExitCodes control_game(FILE*** readPipes, FILE*** writePipes,
	int playerCount, char* deck, char* path);

/* Takes in the game representation, the (validated) deck file contents, the
 * (validated) path file contents, the read and write pipes, and a flag to
 * identify if a player or the dealer called particular functions that both
 * players and the dealer can call. This flag should be passed as false.
 * Communicates with the player via string messages, and processes messages
 * received. Returns the appropriate dealer exit code. */
DealerExitCodes send_and_receive_messages(Game* game, char* deck, char* path,
	FILE*** readPipes, FILE*** writePipes, bool playerCalled);

/* Takes in the player count. Ensure program does not use default signal
 * handlers. */
void setup_signal_handling(int playerCount);

/* Takes in a signal from the kernel (SIGHUP specifically). Updates the global
 * variable flag to identify whether SIGHUP has been received to true. */
void kill_and_reap_children(int signal);

/* Takes in the game representation and returns the player ID of the player
 * who should move next. */
int calculate_whose_turn(Game* game);

/* Takes in the game representation and returns whether the game is over. */
bool is_game_over(Game* game);

/* Takes in the game representation, the ID of the moving player, the site
 * that they would like to move to, and the deck. Returns the HAP message to
 * send to the player to execute. */
char* create_hap_message(Game* game, int movingPlayer, int newSite,
	char* deck);

/* Takes in the game representation and the (validated) deck in the given file
 * format. Returns the next card to be drawn (in the format required by the
 * HAP message). */
CardType draw_next_card(Game* game, char* deck);

/* Takes in the write pipes, the game representation, and the path. Handles
 * clean up of early game over. */
void handle_early_game_over(FILE*** writePipes, Game* game,
	char* path);

/* Takes in the read and write pipes, as well as the player count. Closes
 * each pipe and frees the memory associated to the collections of read and
 * write pipes. */
void free_and_close_pipes(FILE** readPipes, FILE** writePipes,
	int playerCount);

#endif
