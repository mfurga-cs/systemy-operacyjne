#pragma once

#define GAMES_MAX 64
#define GAME_MAX_NICK 64

#define CONNECTION_NONE 0
#define CONNECTION_UDP 1
#define CONNECTION_UNIX 2

typedef struct {
  int type;
  union {
    int port;
    char path[108];
  } conn;
} connection_t;

typedef struct {
  connection_t *player_id[2];
  char player_nick[2][GAME_MAX_NICK];
  char player_sign[2];
  char board[9];
  connection_t *turn;
  int active;
} game_t;

typedef struct {
  game_t games[GAMES_MAX];
  int actives;
} games_t;

int connection_cmp(connection_t *c1, connection_t *c2);

void games_init(games_t *g);

game_t *games_get(games_t *g, connection_t *pid);

game_t *games_add(games_t *g, connection_t *pid1,
                  connection_t *pid2, char *pnick1, char *pnick2);

int games_nick_available(games_t *g, char *nick);

int game_move(game_t *g, connection_t *player, int move);

int game_end(game_t *g);

size_t game_info(game_t *g, char *buff, size_t size);

