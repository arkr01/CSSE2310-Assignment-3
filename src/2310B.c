#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "playerErrors.h"
#include "2310X.h"

/* Takes in the game representation and this player's representation.
 * Calculates the appropriate next move to make based on the Player B
 * strategy. Returns the site number of the next move to be made. */
int calculate_type_b_move(Game* game, Player* thisPlayer);

/* Takes in the game representation. Returns the player ID of the player who
 * has the most cards. If all players have zero cards, returns the player
 * count. Otherwise, returns INVALID_PLAYER_ID. */
int calculate_player_with_max_cards(Game* game);

int main(int argc, char** argv) {
    // Set following to NULL, will be populated in setup_player()
    char* path = NULL;
    Game* game = NULL;
    Player* thisPlayer = NULL;

    PlayerExitCodes playerError = setup_player(argc, argv, &game, &thisPlayer,
	    &path);

    if (playerError != PLAYER_NORMAL) {
	return playerError;
    }

    playerError = play_game(game, thisPlayer, calculate_type_b_move);
    free_game(game, path);
    return player_error_message(playerError);
}

int calculate_type_b_move(Game* game, Player* thisPlayer) {
    int nextSite = thisPlayer->currentSite + 1;
    // Ensure all players are in front of thisPlayer
    int player = 0;
    for (; player < game->playerCount; player++) {
	if (game->players[player]->currentSite <= thisPlayer->currentSite &&
		player != thisPlayer->playerID) {
	    break;
	}
    }
    // If loop completed normally (i.e. break was never called), then all
    // players are in front of thisPlayer
    if (player == game->playerCount && !check_site_full(game, nextSite)) {
	return nextSite;
    }

    // Check for odd amount of money and find first available Mo site if this
    // is the case.
    int firstValidMoSite = get_first_site_of_type(SITE_MO, thisPlayer, game);
    if (thisPlayer->money % 2 && firstValidMoSite != INVALID_SITE) {
	return firstValidMoSite;
    }

    // Below func returns the playerCount if everyone has 0 cards.
    int playerWithMaxCards = calculate_player_with_max_cards(game);
    int firstValidRiSite = get_first_site_of_type(SITE_RI, thisPlayer, game);

    if ((playerWithMaxCards == thisPlayer->playerID || playerWithMaxCards ==
	    game->playerCount) && firstValidRiSite != INVALID_SITE) {
	return firstValidRiSite;
    }

    int firstValidV2Site = get_first_site_of_type(SITE_V2, thisPlayer, game);
    if (firstValidV2Site != INVALID_SITE) {
	return firstValidV2Site;
    }

    for (int site = nextSite; site < game->path->numSites; site++) {
	if (!check_site_full(game, site) &&
		!check_barrier_skipped(game, thisPlayer, site)) {
	    return site;
	}
    }
    return INVALID_SITE; // Should never reach here, something went wrong
}

int calculate_player_with_max_cards(Game* game) {
    int mostCards = 0;
    int playerWithMostCards = INVALID_PLAYER_ID;
    bool tieForMax = false; // Flag to ensure player has strict max # of cards

    for (int player = 0; player < game->playerCount; player++) {
	int numCardsOfPlayer = 0;
	for (int cardType = 0; cardType < NUM_CARD_TYPES; cardType++) {
	    numCardsOfPlayer += game->players[player]->numCards[cardType];
	}
	if (numCardsOfPlayer > mostCards) {
	    mostCards = numCardsOfPlayer;
	    playerWithMostCards = player;
	    tieForMax = false;
	} else if (numCardsOfPlayer == mostCards) {
	    tieForMax = true;
	}
    }
    
    // If all players have 0 cards, return the player count as a sentinel
    // Otherwise, if no player has a strict maximum number of cards, return
    // INVALID_PLAYER_ID as a sentinel
    if (!mostCards && tieForMax) {
	playerWithMostCards = game->playerCount;
    } else if (tieForMax) {
	playerWithMostCards = INVALID_PLAYER_ID;
    }

    return playerWithMostCards;
}
