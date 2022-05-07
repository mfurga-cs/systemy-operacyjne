#pragma once

#define SHM_KEY_PATHNAME "/dev/zero"
#define SHM_KEY_PROID 100
#define SHM_SIZE 1024

void *shmem_init(int proid) {
  key_t key = ftok(SHM_KEY_PATHNAME, proid);
  if (key == -1) {
    puts("ftok failed.");
    return NULL;
  }

  int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0777);
  if (shmid == -1) {
    puts("shmget failed.");
    return NULL;
  }

  void *shmem = shmat(shmid, NULL, 0);
  if (shmem == (void *)-1) {
    puts("shmat failed.");
    return NULL;
  }

  return shmem;
}


void shmem_del(int proid) {
  key_t key = ftok(SHM_KEY_PATHNAME, proid);
  if (key == -1) {
    puts("ftok failed.");
    return;
  }

  int shmid = shmget(key, 0, 0);
  if (shmid == -1) {
    puts("shmget failed.");
    return;
  }

  shmctl(shmid, IPC_RMID, NULL);
}