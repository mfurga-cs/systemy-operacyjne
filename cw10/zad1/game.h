#pragma once

#define GAMES_MAX 64
#define GAME_MAX_NICK 64

typedef struct {
  int player_id[2];
  char player_nick[2][GAME_MAX_NICK];
  char player_sign[2];
  char board[9];
  int turn;
  int active;
} game_t;

typedef struct {
  game_t games[GAMES_MAX];
  int actives;
} games_t;

void games_init(games_t *g);

game_t *games_get(games_t *g, int pid);

game_t *games_add(games_t *g, int pid1, int pid2,
                  char *pnick1, char *pnick2);

int games_nick_available(games_t *g, char *nick);

int game_move(game_t *g, int player, int move);

int game_end(game_t *g);

size_t game_info(game_t *g, char *buff, size_t size);

