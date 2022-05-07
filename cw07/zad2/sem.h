#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

sem_t *sem_create(const char *name, int value) {
  sem_t *sem = sem_open(name, O_CREAT | O_RDWR, 0777, value);
  if (sem == SEM_FAILED) {
    puts("sem_open failed.");
    return NULL;
  }
  return sem;
}

void sem_del(const char *name, sem_t *sem) {
  sem_close(sem);
  sem_unlink(name);
}

// void sem_wait(sem_t *sem) {
//   sem_wait(sem);
// }

void sem_signal(sem_t *sem) {
  sem_post(sem);
}

