#ifndef GENERAL_PLAYER_H
#define GENERAL_PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "playerErrors.h"
#include "dealerErrors.h"

/* The number of site types */
#define NUM_SITE_TYPES 6

/* The number of card types */
#define NUM_CARD_TYPES 5

/* The get_line() function re-allocates memory if necessary. Hence, whenever
 * using get_line, let us begin with an initial buffer size of 30 bytes. */
#define INITIAL_BUFFER_SIZE 30

/* The get_line() function re-allocates memory if necessary. Below provides a
 * re-sizing factor to scale the memory re-allocation appropriately. */
#define RESIZING_FACTOR 1.5

/* Every path starts and ends with barrier sites. Hence, the minimum number of
 * sites is 2. */
#define MIN_SITES 2

/* In a valid game path, each site is 3 chars long (2 chars for the site type
 * and 1 char for the site player limit (in file)/space (in game)/null
 * terminator (in internal storage of site type, i.e. it is a string).) */
#define SITE_LENGTH 3

/* The player ID must be non-negative. Hence, for error-checking, we may use
 * the value -1 as a sentinel. In this case, we will use it to represent parts
 * of the path that players are not currently at. */
#define INVALID_PLAYER_ID (-1)

/* The player ID must be non-negative. Hence, for error-checking, we may use
 * the value -2 as a sentinel. The game display is presented in a grid-form,
 * where the rows represent each player, and the columns represent each char
 * in a site representation (i.e. including spaces). An empty row shall begin
 * with a sentinel value of -2. */
#define EMPTY_LINE (-2)

/* Site number must be non-negative. For error-checking, we may use the value
 * -3 as a sentinel. For example, when calculating the next site a player
 * should move to, this value may be used if no site is found. */
#define INVALID_SITE (-3)

/* Site representation */
typedef struct {
    char type[SITE_LENGTH];
    int limit;
    int* playersAtSite;
} Site;

/* Path representation */
typedef struct {
    int numSites;
    Site* sites;
} Path;

/* Player representation */
typedef struct {
    int playerID;
    int money;
    int numPoints;
    int currentSite; 
    int numV1SitesVisited;
    int numV2SitesVisited;
    
    // Each element stores number of each type of card drawn by this player.
    // e.g. numCards[0] == number of A cards drawn, numCards[1] == number of B
    // cards drawn, etc.
    int numCards[NUM_CARD_TYPES];
} Player;

/* Game representation */
typedef struct {
    Path* path;
    Player** players;
    int playerCount;
} Game;

/* Message Types */
typedef enum {
    MESSAGE_YT = 0,
    MESSAGE_DO = 1,
    MESSAGE_EARLY = 2,
    MESSAGE_DONE = 3,
    MESSAGE_HAP = 4,
    MESSAGE_ERROR = 5
} MessageType;

/* Site Types */
typedef enum {
    SITE_MO = 0,
    SITE_V1 = 1,
    SITE_V2 = 2,
    SITE_DO = 3,
    SITE_RI = 4,
    SITE_BARRIER = 5,
    SITE_ERROR = 6
} SiteType;

/* Components of HAP message. */
typedef enum {
    MOVE_PLAYER_ID = 0,
    MOVE_NEW_SITE = 1,
    MOVE_ADDITIONAL_POINTS = 2,
    MOVE_MONEY_CHANGE = 3,
    MOVE_CARD_DRAWN = 4
} HapComponent;

/* Entry point for Player A and Player B programs. Essentially acts as main.
 * Takes in the same parameters as main, as well as unitialised game and
 * player representations, along with an empty buffer to store the path.
 * Handles all necessary setup prior to starting the game. Returns the
 * appropriate player exit code. */
PlayerExitCodes setup_player(int argc, char** argv, Game** game,
	Player** thisPlayer, char** path);

/* Takes in the (validated) path from the given path file, as well as the
 * player count. Initialises and returns the game representation. Entry
 * point/wrapper function for initialisation of game representation members.
 * */
Game* init_game(char* pathFromFile, int playerCount);

/* Takes in the game representation and the (validated) path from the given
 * path file. Initialises the game path representation. */
void init_game_path(Game* game, char* pathFromFile);

/* Takes in the game representation. Initialises the component of the site
 * representation that tracks which players are on the site. */
void init_game_site_players(Game* game);

/* Takes in the game representation. Malloc's memory for each player and
 * initialises the player representation information (e.g. initial amount of
 * money, initial position on path, etc.). Does not specify player type. */
void init_game_players(Game* game);

/* Takes in the input converted via strtol call, as well as the error
 * stored via strtol call, and checks (and returns) if the input was
 * invalid. */
bool strtol_invalid(char* input, char* error);

/* Takes in an empty buffer, its max size to store the path, the source of the
 * path, and a flag to check if the player or the dealer is calling this
 * function. Reads and validates the path from the given path source
 * and returns the appropriate player/dealer exit code. Forms as entry
 * point/wrapper function for all path error handling. */
int validate_path(char** pathFromFile, size_t* pathLength, FILE* pathSource,
	bool playerCalled);

/* Takes in a buffer to store the line read, an initial minimum length of the
 * line to be read, and the source of the line to be read. Reads in a single
 * line of input and stores in the buffer. If the line of input is longer than
 * the minimum length provided, more space is allocated to store the remainder
 * of the line. Returns if valid line could be read (e.g. no unexpected EOF).
 * */
bool get_line(char** buffer, size_t* minBufferSize, FILE* sourceOfLine);

/* Takes in the game representation, this player's representation, and a
 * message from either the dealer or the player. Returns the appropriate
 * message type. */
MessageType get_message_type(Game* game, Player* thisPlayer,
	char* dealerOrPlayerMessage);

/* Takes in the path representation of a site and returns the appropriate site
 * type. */
SiteType get_site_type(char* site);

/* Takes in a site type to search the path for, this player's representation,
 * and the game representation. Finds the first occurrence of the given site
 * type (in front of the player) on the path, ensuring that no barriers are
 * skipped and that the site is not full, and returns the site number. If no
 * such occurrence is found, INVALID_SITE is returned. */
int get_first_site_of_type(SiteType siteType, Player* thisPlayer, Game* game);

/* Takes in the game representation, this player's representation, and the
 * dealer/player message. Checks if message is a DO message, and returns if it
 * is a valid DO move. Does not check player strategies. */
bool do_message_valid(Game* game, Player* thisPlayer,
	char* dealerOrPlayerMessage);

/* Takes in the game representation, and the dealer/player message. Checks if
 * message is a HAP message and checks (and returns) if said message is valid.
 * Does not check if said message is the correct action to be taken, only
 * checks if message contents are possible at *some* point in the game (e.g.
 * ensures message refers to a player that exists in the game, does not check
 * if said player should have moved at that point in time). */
bool hap_message_valid(Game* game, char* dealerOrPlayerMessage);

/* Takes in the game representation, the player ID in the HAP message, and a
 * value from the HAP message to validate, as well as the type of value (i.e.
 * which component of the HAP message it is, e.g. player ID, new site, etc.).
 * Returns if said value is invalid (e.g. does the player ID refer to a player
 * that exists?) */
bool hap_message_number_invalid(Game* game, int playerID, int valueToCheck,
	HapComponent valueType);

/* Takes in an empty buffer to store errors, as well as the section (and the
 * remainder, see note) of the HAP message to check. Checks said section for
 * invalid chars (including the case where no chars, valid or invalid, have
 * been provided at all), and returns if invalid chars are found.
 * NOTE: this function is designed to check the remainder of the HAP message.
 * i.e. if we consider a message 'HAPp,n,s,m,c', if we wish to check for
 * invalid chars in s, we would pass 's,m,c' into this function. */
bool hap_invalid_chars(char* remainderOfHap, char* errorBuffer);

/* Takes in the game representation, a (validated) HAP message, and a flag to
 * check if a player or the dealer called this function. This function
 * processes the given updates to the game state (i.e. the player ID, the new
 * site, the change in points (if any), the change in money (if any), and the
 * card drawn (if any), of the player who has just moved). */
void process_hap_details(Game* game, char* hapMessage, bool playerCalled);

/* Takes in the game representation, the player representation of the moving
 * player, the site that the player is moving from, and the site that the
 * player is moving to. Updates both sites regarding which players are at said
 * sites. */
void update_player_sites(Game* game, Player* movingPlayer, int originalSite,
	int newSite);

/* Takes in the game representation, the representation of the player who has
 * just made a move, and where to display output. Displays information about
 * said player. */
void display_player_details(Game* game, Player* thisPlayer,
	FILE* displayLocation);

/* Takes in the game representation, and the move that the player would like
 * to make. Checks (and returns) if the site the player would like to move to
 * has room. */
bool check_site_full(Game* game, int move);

/* Takes in the game representation, the moving player's representation, and
 * the move that said player would like to make. Checks (and returns) if the
 * site the player would like to move to skips a barrier site. */
bool check_barrier_skipped(Game* game, Player* movingPlayer, int move);

/* Takes in a string and a character to count, and counts (and returns) the
 * number of times said character appears in said string. */
int character_counter(char* stringToSearch, char characterToCount);

/* Takes in a string to reverse, as well as the length of said string
 * (excluding the null terminator). Returns the reversed string.
 * NOTE: malloc's memory for the returned string. */
char* reverse_string(char* toReverse, size_t toReverseLength);

/* Takes in the path from the given path file, excluding the number of site
 * types, as well as a flag to check if the player or the dealer is calling
 * this function. Validiates path contents. Returns the appropriate
 * player/dealer exit code. */
int validate_path_sites(char* pathSitesFromFile, bool playerCalled);

/* Takes in the path from the given path file, excluding the number of site
 * types and the semi-colon, as well as a flag to check if the player or the
 * dealer is calling this function. Ensures each site type in path is valid;
 * returns the appropriate player/dealer exit code. */
int validate_site_types(char* pathSitesFromFile,
	bool playerCalled);

/* Takes in the path from the given path file, excluding the number of site
 * types and the semi-colon, as well as a flag to check if the player or the
 * dealer is calling this function. Ensures each site's player limit in path
 * is valid, and returns the appropriate player exit code. Assumes site types
 * have been validated. */
int validate_site_player_limits(char* pathSitesFromFile, bool playerCalled);

/* Takes in the game representation, the player representation of this player,
 * their move strategy, and a flag to check if a player or the dealer called
 * this function. Entry point for game after path and command-line argument
 * validation is complete. Returns the appropriate player exit code. */
PlayerExitCodes play_game(Game* game, Player* thisPlayer,
	int (*moveStrategy)(Game* game, Player* thisPlayer));

/* Takes in the game representation, and a flag to check if a player or the
 * dealer called this function. Calculates and displays the final scores
 * for each player in the required format (i.e. in player order,
 * comma-separated, and newline-terminated). */
void calculate_final_scores(Game* game, bool playerCalled);

/* Takes in a player's representation and calculates the number of points
 * obtained from the cards the player has. */
void calculate_score_from_cards(Player* player);

/* Takes in the game representation, as well as a flag to check if a player or
 * the dealer called this function. Presents the path and the players in the
 * required game format. */
void display_game(Game* game, bool playerCalled);

/* Takes in the game representation. Creates (and returns) a matrix
 * representation of the player positions. The matrix takes dimensions
 * (number of players) x (number of chars in path). Each element is the player
 * ID, if that player is at that particular position of the path. If no player
 * exists at a particular part of the display, the element is of value
 * INVALID_PLAYER_ID. */
int** init_player_positions(Game* game);

/* Takes in the game representation and the matrix representation of the
 * player positions. Sorts the matrix to ensure the players at each site are
 * displayed as close to the game path as possible (i.e. no blank spaces
 * above/below). */
void sort_player_positions(Game* game, int*** playerDisplay);

/* Takes in the game representation and the matrix representation of the
 * player positions. Modifies the matrix to ensure that unnecessary spacing
 * is not present in the final display. This should be called after
 * sort_player_positions. */
void clean_player_display(Game* game, int*** playerDisplay);

/* Takes in the game representation and the (validated) path from the given
 * path file. Frees the player representations, the game path site
 * representations, the game path representation, and the (validated) path
 * from the given path file. */
void free_game(Game* game, char* pathFromFile);

#endif
