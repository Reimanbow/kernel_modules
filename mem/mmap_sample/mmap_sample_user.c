/**
 * @file mmap_sample_user.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#define MEM_SIZE 4096

int main(void) {
	int fd;
	char *mapped_mem;
	
	fd = open("/dev/mmap_test", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	/* mmap()でカーネルメモリをマッピング */
	mapped_mem = (char *)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mapped_mem == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return -1;
	}

	/* メモリにデータを書き込む */
	strcpy(mapped_mem, "Hello from user space!");

	/* 読み取り */
	printf("Read from mmap memory: %s\n", mapped_mem);

	/* マッピングを解除 */
	munmap(mapped_mem, MEM_SIZE);
	close(fd);
	return 0;
}