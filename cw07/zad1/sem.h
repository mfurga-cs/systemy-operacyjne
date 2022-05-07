#pragma once

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEM_KEY_PATHNAME "/dev/null"
#define SEM_KEY_PROID 100

#define SEM_SIZE 10

void sem_wait(int semid, int id) {
  struct sembuf op = {
    .sem_num = id, .sem_op = -1, .sem_flg = 0
  };
  semop(semid, &op, 1);
}

void sem_signal(int semid, int id) {
  struct sembuf op = {
    .sem_num = id, .sem_op = 1, .sem_flg = 0
  };
  semop(semid, &op, 1);
}

int sem_init(void) {
  key_t key = ftok(SEM_KEY_PATHNAME, SEM_KEY_PROID);
  if (key == -1) {
    puts("ftok failed.");
    return -1;
  }
  int semid = semget(key, SEM_SIZE, IPC_CREAT | 0777);
  if (semid == -1) {
    puts("semget failed.");
    return -1;
  }
  return semid;
}

void sem_del(int semid) {
  semctl(semid, 0, IPC_RMID);
}

void sem_set(int semid, int id, int val) {
  union {
    int val;
  } semun = { .val = val };
  semctl(semid, id, SETVAL, semun);
}

int sem_get(int semid, int id) {
  return semctl(semid, id, GETVAL);
}
