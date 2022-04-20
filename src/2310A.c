#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "playerErrors.h"
#include "2310X.h"

/* Takes in the game representation and this player's representation.
 * Calculates the appropriate next move to make based on the Player A
 * strategy. Returns the site number of the next move to be made. */
int calculate_type_a_move(Game* game, Player* thisPlayer);

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

    playerError = play_game(game, thisPlayer, calculate_type_a_move);
    free_game(game, path);
    return player_error_message(playerError);
}

int calculate_type_a_move(Game* game, Player* thisPlayer) {
    int nextSite = thisPlayer->currentSite + 1;

    // If player has money, and there is a Do site in front, go there (ensure
    // site is not full and no barriers are skipped)
    int firstValidDoSite = get_first_site_of_type(SITE_DO, thisPlayer, game);
    if (thisPlayer->money > 0 && firstValidDoSite != INVALID_SITE) {
	return firstValidDoSite;
    }

    // If next site is Mo, go there (ensure site is not full)
    if (get_site_type(game->path->sites[nextSite].type) == SITE_MO &&
	    !check_site_full(game, nextSite)) {
	return nextSite;
    }

    // Go to the nearest V1, V2, or barrier site (::) (ensure site is not full
    // and no barriers are skipped)
    for (int site = nextSite; site < game->path->numSites; site++) {
	SiteType siteToMoveTo = get_site_type(game->path->sites[site].type);
	if ((siteToMoveTo == SITE_V1 || siteToMoveTo == SITE_V2 ||
		siteToMoveTo == SITE_BARRIER) &&
		!check_site_full(game, site) &&
		!check_barrier_skipped(game, thisPlayer, site)) {
	    return site;
	}
    }
    return INVALID_SITE; // Should never reach here, something went wrong
}
