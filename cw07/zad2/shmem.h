#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHM_SIZE 1024

void *shmem_init(const char *name) {
  int shmid = shm_open(name, O_CREAT | O_RDWR, 0777);
  if (shmid == -1) {
    puts("shm_open failed.");
  }

  if (ftruncate(shmid, SHM_SIZE) == -1) {
    puts("ftruncate failed.");
  }

  void *p = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
  if (p == (void *)-1) {
    puts("mmap failed.");
  }

  return p;
}

void shmem_del(const char *name, void *p) {
  munmap(p, SHM_SIZE);
  shm_unlink(name);
}