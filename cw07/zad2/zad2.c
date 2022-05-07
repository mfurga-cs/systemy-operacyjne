#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include "sem.h"
#include "shmem.h"

#define PIZZA_BUFF_SIZE 5

typedef struct {
  int pizzas[PIZZA_BUFF_SIZE];
  int size;

  sem_t *sem_empty;
  sem_t *sem_full;
  sem_t *sem_mutex;

} pizza_buff_t;

typedef struct {
  int index;
  int type;
  int size;
} response_t;

pizza_buff_t *bake;
pizza_buff_t *table;

long long current_timestamp(void) {
  struct timeval te; 
  gettimeofday(&te, NULL);
  return (long long)te.tv_sec * 1000LL + (long long)te.tv_usec / 1000LL;
}

void pizza_buff_init(void) {
  bake = shmem_init("/bake");
  if (bake == NULL) {
    puts("shmem_init failed.");
  }

  bake->size = 0;

  bake->sem_empty = sem_create("/bake_empty", PIZZA_BUFF_SIZE);
  bake->sem_full = sem_create("/bake_full", 0);
  bake->sem_mutex = sem_create("/bake_mutex", 1);

  table = shmem_init("/table");
  if (table == NULL) {
    puts("shmem_init failed.");
  }

  table->size = 0;

  table->sem_empty = sem_create("/table_empty", PIZZA_BUFF_SIZE);
  table->sem_full = sem_create("/table_full", 0);
  table->sem_mutex = sem_create("/table_mutex", 1);

  for (int i = 0; i < PIZZA_BUFF_SIZE; i++) {
    bake->pizzas[i] = -1;
    table->pizzas[i] = -1;
  }
}

void pizza_buff_del(void) {
  sem_del("/bake_empty", bake->sem_empty);
  sem_del("/bake_full", bake->sem_full);
  sem_del("/bake_mutex", bake->sem_mutex);

  sem_del("/table_empty", table->sem_empty);
  sem_del("/table_full", table->sem_full);
  sem_del("/table_mutex", table->sem_mutex);

  shmem_del("/bake", bake);
  shmem_del("/table", table);
}

response_t pizza_buff_add(pizza_buff_t *buff, int type) {

  sem_wait(buff->sem_empty);
  sem_wait(buff->sem_mutex);

  response_t r;

  for (int i = 0; i < PIZZA_BUFF_SIZE; i++) {
    if (buff->pizzas[i] == -1) {
      r.index = i;
      break;
    }
  }
  buff->pizzas[r.index] = type;
  r.size = ++buff->size;
  r.type = type;

  sem_signal(buff->sem_mutex);
  sem_signal(buff->sem_full);

  return r;
}

response_t pizza_buff_remove(pizza_buff_t *buff, int idx) {
  response_t r;

  sem_wait(buff->sem_full);
  sem_wait(buff->sem_mutex);

  if (idx == -1) {
    for (int i = 0; i < PIZZA_BUFF_SIZE; i++) {
      if (buff->pizzas[i] != -1) {
        idx = i;
        break;
      }
    }
  }

  assert(buff->pizzas[idx] != -1);

  r.type = buff->pizzas[idx];
  buff->pizzas[idx] = -1;
  r.size = --buff->size;

  sem_signal(buff->sem_mutex);
  sem_signal(buff->sem_empty);

  return r;
}

void pizza_buff_info(pizza_buff_t *buff) {
  printf("mapped at: %p\n", buff);
  for (int i = 0; i < PIZZA_BUFF_SIZE; i++) {
    printf("%d ", buff->pizzas[i]);
  }
  printf("\nsize: %u\n\n", buff->size);
}

void cook(void) {
  response_t r;

  while (1) {
    int type = rand() % 10;

    printf("[%d %lld] Przygotowuje pizze: %d.\n", getpid(), current_timestamp(), type);
    sleep((rand() % 2) + 1);

    r = pizza_buff_add(bake, type);
    printf("[%d %lld] Dodalem pizze: %d. Liczba pizz w piecu: %d.\n",
      getpid(), current_timestamp(), type, r.size);
    sleep((rand() % 2) + 4);

    r = pizza_buff_remove(bake, r.index);

    int in_bake = r.size;
    r = pizza_buff_add(table, type);
    int in_table = r.size;

    printf("[%d %lld] Wyjmuje pizze: %d. Liczba pizz w piecu: %d. Liczba pizz na stole: %d.\n",
      getpid(), current_timestamp(), type, in_bake, in_table);
  }
}

void deliver(void) {
  response_t r;

  while (1) {
    r = pizza_buff_remove(table, -1);
    printf("[%d %ld] Pobieram pizze: %d Liczba pizz na stole: %d.\n",
      getpid(), time(NULL), r.type, r.size);

    sleep((rand() % 2) + 4);
    printf("[%d %lld] Dostarczam pizze: %d.\n", getpid(), current_timestamp(), r.type);

    sleep((rand() % 2) + 4);
  }
}

void handle_SIGINT(int sig) {
  (void)sig;
  pizza_buff_del();
  _exit(0);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  srand(time(NULL));

  if (argc != 3) {
    printf("Usage: %s <N> <M>\n", argv[0]);
    return 1;
  }

  int n = atoi(argv[1]);
  int m = atoi(argv[2]);

  pizza_buff_init();

  pid_t cooks[n];
  pid_t suppliers[m];

  for (int i = 0; i < n; i++) {
    cooks[i] = fork();
    if (cooks[i] == 0) {
      setvbuf(stdout, NULL, _IONBF, 0);
      cook();
      return 1;
    }
  }

  for (int i = 0; i < m; i++) {
    suppliers[i] = fork();
    if (suppliers[i] == 0) {
      setvbuf(stdout, NULL, _IONBF, 0);
      deliver();
      return 1;
    }
  }

  signal(SIGINT, handle_SIGINT);

  for (int i = 0; i < n; i++) {
    waitpid(cooks[i], NULL, 0);
  }

  for (int i = 0; i < n; i++) {
    waitpid(suppliers[i], NULL, 0);
  }

  pizza_buff_del();

  return 0;
}

