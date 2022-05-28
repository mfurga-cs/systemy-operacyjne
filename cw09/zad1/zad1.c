#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define RAND_RANGE(a, b) ((rand() % ((b) - (a) + 1)) + a)
#define ELF_AMOUNT 10
#define REINDEER_AMOUNT 9

atomic_int elve_next_id;
atomic_int reindeer_next_id;

pthread_mutex_t mutex_santa_claus = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_santa_claus = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_elve = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_elve = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_reindeer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_reindeer = PTHREAD_COND_INITIALIZER;

int elves_waiting[3] = {};
int elves_waiting_size = 0;
int elves_waiting_idx = 0;

int reindeers_waiting = 0;

void *santa_claus(void *p) {
  (void)p;

  int delivered_toys = 0;

  while (1) {
    pthread_mutex_lock(&mutex_santa_claus);
    while (elves_waiting_size < 3 && reindeers_waiting < 9) {
      pthread_cond_wait(&cond_santa_claus, &mutex_santa_claus);
    }
    puts("Mikolaj: budze sie");
    pthread_mutex_unlock(&mutex_santa_claus);

    pthread_mutex_lock(&mutex_elve);
    if (elves_waiting_size == 3) {
      pthread_mutex_unlock(&mutex_elve);

      printf("Mikolaj: rozwiazuje problemy elfow (%d, %d, %d)\n",
        elves_waiting[0], elves_waiting[1], elves_waiting[2]);

      for (int i = 0; i < 3; i++) {
        printf("Elf(%d): Mikolaj rozwiazuje problem\n", elves_waiting[i]);
        sleep(RAND_RANGE(1, 2));
      }

      pthread_mutex_lock(&mutex_elve);
      elves_waiting_size = 0;
      pthread_cond_broadcast(&cond_elve);
    }
    pthread_mutex_unlock(&mutex_elve);

    pthread_mutex_lock(&mutex_reindeer);
    if (reindeers_waiting == 9) {
      delivered_toys++;
      printf("Mikolaj: dostarczam zabawki (%d)\n", delivered_toys);
      sleep(RAND_RANGE(2, 4));
      reindeers_waiting = 0;
      pthread_cond_broadcast(&cond_reindeer);
    }
    pthread_mutex_unlock(&mutex_reindeer);

    if (delivered_toys == 3) {
      puts("Mikolaj: Dostarczylem 3 prezenty. Koniec na dzis");
      exit(0);
    }

    puts("Mikolaj: zasypiam");
  }
}

void *elve(void *p) {
  (void)p;

  int id = atomic_fetch_add(&elve_next_id, 1);

  while (1) {
    sleep(RAND_RANGE(2, 5));

    pthread_mutex_lock(&mutex_elve);

check:
    if (elves_waiting_size < 3) {
      elves_waiting_size++;
      elves_waiting[elves_waiting_idx] = id;
      elves_waiting_idx = (elves_waiting_idx + 1) % 3;

      printf("Elf(%d): czeka %d elfow na Mikolaja\n", id, elves_waiting_size);

      if (elves_waiting_size == 3) {
        printf("Elf(%d): wybudzam Mikolaja\n", id);
        pthread_cond_broadcast(&cond_santa_claus);
      }

      pthread_cond_wait(&cond_elve, &mutex_elve);
      pthread_mutex_unlock(&mutex_elve);
   } else {
      printf("Elf(%d): czeka na powrot elfow\n", id);
      pthread_cond_wait(&cond_elve, &mutex_elve);
      goto check;
    }
  }
}

void *reindeer(void *p) {
  (void)p;

  int id = atomic_fetch_add(&reindeer_next_id, 1);

  while (1) {
    sleep(RAND_RANGE(5, 10));

    pthread_mutex_lock(&mutex_reindeer);

    reindeers_waiting++;
    printf("Renifer(%d): czeka %d reniferow na Mikolaja\n", id, reindeers_waiting);

    if (reindeers_waiting == 9) {
      printf("Renifer(%d): wybudzam Mikolaja\n", id);
      pthread_cond_broadcast(&cond_santa_claus);
    }

    pthread_cond_wait(&cond_reindeer, &mutex_reindeer);
    pthread_mutex_unlock(&mutex_reindeer);
  }
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  srand(time(NULL));

  pthread_t thread_santa_claus;
  pthread_t thread_elve[ELF_AMOUNT];
  pthread_t thread_reindeer[REINDEER_AMOUNT];

  if (pthread_create(&thread_santa_claus, NULL, santa_claus, NULL) != 0) {
    puts("pthread_create failed.");
    return 1;
  }

  for (int i = 0; i < ELF_AMOUNT; i++) {
    if (pthread_create(&thread_elve[i], NULL, elve, NULL) != 0) {
      puts("pthread_create failed.");
      return 1;
    }
  }

  for (int i = 0; i < REINDEER_AMOUNT; i++) {
    if (pthread_create(&thread_reindeer[i], NULL, reindeer, NULL) != 0) {
      puts("pthread_create failed.");
      return 1;
    }
  }

  void *ret;
  pthread_join(thread_santa_claus, &ret);

  for (int i = 0; i < ELF_AMOUNT; i++) {
    pthread_join(thread_elve[i], &ret);
  }

  for (int i = 0; i < ELF_AMOUNT; i++) {
    pthread_join(thread_elve[i], &ret);
  }

  return 0;
}

