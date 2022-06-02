#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "game.h"

int connection_cmp(connection_t *c1, connection_t *c2) {
  if (c1->type != c2->type) {
    return 0;
  }
  if (c1->type == CONNECTION_UDP) {
    return c1->conn.port == c2->conn.port;
  } else {
    return strcmp(c1->conn.path, c2->conn.path) == 0;
  }
}

void games_init(games_t *g) {
  for (int i = 0; i < GAMES_MAX; i++) {
    g->games[i].active = 0;
  }
  g->actives = 0;
}

game_t *games_get(games_t *g, connection_t *pid) {
  assert(pid->type != CONNECTION_NONE);

  for (int i = 0; i < GAMES_MAX; i++) {
    if (!g->games[i].active) {
      continue;
    }
    if (connection_cmp(g->games[i].player_id[0], pid) ||
        connection_cmp(g->games[i].player_id[1], pid)) {
      return &g->games[i];
    }
  }
  return NULL;
}

game_t *games_add(games_t *g, connection_t *pid1,
                  connection_t *pid2, char *pnick1, char *pnick2) {

  assert(games_get(g, pid1) == NULL);
  assert(games_get(g, pid2) == NULL);

  game_t *gg = NULL;
  for (int i = 0; i < GAMES_MAX; i++) {
    if (!g->games[i].active) {
      gg = &g->games[i];
    }
  }

  assert(gg != NULL);

  gg->active = 1;

  gg->player_id[0] = pid1;
  gg->player_id[1] = pid2;
  gg->player_sign[0] = 'X';
  gg->player_sign[1] = 'O';
  memcpy(gg->player_nick[0], pnick1, strlen(pnick1));
  memcpy(gg->player_nick[1], pnick2, strlen(pnick2));

  gg->player_nick[0][strlen(pnick1)] = '\0';
  gg->player_nick[1][strlen(pnick2)] = '\0';

  memset(gg->board, ' ', sizeof(gg->board));
  /*
  for (int i = 1; i <= 9; i++) {
    gb[i - 1] = '0' + i;
  }
  */
  gg->turn = pid1;
  return gg;
}

int games_nick_available(games_t *g, char *nick) {
  for (int i = 0; i < GAMES_MAX; i++) {
    if (!g->games[i].active) {
      continue;
    }
    if (strcmp(nick, g->games[i].player_nick[0]) == 0 ||
        strcmp(nick, g->games[i].player_nick[1]) == 0) {
      return 0;
    }
  }
  return 1;
}

int game_move(game_t *g, connection_t *player, int move) {
  if (move < 1 || move > 9) {
    return 1;
  }

  move--;

  int n = g->player_id[0] == player ? 0 : 1;

  if (g->turn != player) {
    return 1;
  }

  if (g->board[move] == 'X' || g->board[move] == 'O') {
    return 2;
  }

  g->board[move] = g->player_sign[n];
  n = 1 - n;
  g->turn = g->player_id[n];

  return 0;
}

int game_end(game_t *g) {
  char *b = g->board;

  if (
    (b[0] != ' ' && b[0] == b[1] && b[1] == b[2]) ||
    (b[3] != ' ' && b[3] == b[4] && b[4] == b[5]) ||
    (b[6] != ' ' && b[6] == b[7] && b[7] == b[8]) ||

    (b[0] != ' ' && b[0] == b[3] && b[3] == b[6]) ||
    (b[1] != ' ' && b[1] == b[4] && b[4] == b[7]) ||
    (b[2] != ' ' && b[2] == b[5] && b[5] == b[8]) ||

    (b[0] != ' ' && b[0] == b[4] && b[4] == b[8]) ||
    (b[2] != ' ' && b[2] == b[4] && b[4] == b[6])
  ) {
    return 1; // Win.
  }

  for (int i = 0; i < 9; i++) {
    if (b[i] != 'O' && b[i] != 'X') {
      return 0; // Not end yet.
    }
  }

  return 2; // Draw.
}

size_t game_info(game_t *g, char *buff, size_t size) {
  assert(g != NULL);

  size_t curr = 0;
  curr += snprintf(buff + curr, size - curr, "\033c");
  assert(size >= curr);

/*
  printf("player1: %d %s\n", g->player_id[0], g->player_nick[0]);
  printf("player2: %d %s\n", g->player_id[1], g->player_nick[1]);
*/

  curr += snprintf(buff + curr, size - curr,
    "GAME: %s VS %s\n", g->player_nick[0], g->player_nick[1]);
  assert(size >= curr);

  int n = g->player_id[0] == g->turn ? 0 : 1;

  switch (game_end(g)) {
    case 2:
      curr += snprintf(buff + curr, size - curr, "GAME OVER: DRAW!\n");
    break;
    case 1:
      curr += snprintf(buff + curr, size - curr,
        "GAME OVER: %s WON!\n", g->player_nick[1 - n]);
    break;
    case 0:
      curr += snprintf(buff + curr, size - curr,
        "TURN: %s\n", g->player_nick[n]);
    break;
  }
  assert(size >= curr);

  char *b = g->board;

  curr += snprintf(buff + curr, size - curr,
    "   %c | %c | %c \n"
    "   - | - | - \n"
    "   %c | %c | %c \n"
    "   - | - | - \n"
    "   %c | %c | %c \n"
  , b[0], b[1], b[2],
    b[3], b[4], b[5],
    b[6], b[7], b[8]
  );

  return curr;
}

/*
int main(void) {
  games_t games;

  games_init(&games);
  games_add(&games, 0, 1, "mateusz", "ewa");
  game_t *g = games_get(&games, 0);

  char buff[1024];

  int move = 0;
  int c;
  while (!game_end(g)) {
    int s = game_info(g, buff, sizeof(buff));
    buff[s] = '\0';

    printf("%s", buff);

    while ((c = getchar()) == 10);
    c -= '0';
    if (game_move(g, move, c)) {
      move = 1 - move;
    }
  }

  game_info(g, buff, sizeof(buff));
  assert(games_get(&games, 0) != NULL);

  return 0;
}
*/
