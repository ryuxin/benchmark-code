#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define PAGE_SIZE   4096
#define ITER 10000000
int act[ITER], deact[ITER];
extern void pthread_prio(pthread_t pid, unsigned int nice);


static __inline__ unsigned long long
rdtsc (void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

static int comp(const void *a, const void *b)
{
	int *aa = (int *)a;
	int *bb = (int *)b;

	return *bb-*aa;
}

int
main(int argc, char **argv)
{
	int fd, i;
	int *mapdata;
	unsigned long long start, end, tot1, tot2;

	set_prio(0);

	for(i=0; i<ITER; i++) {
		start = rdtsc();
		mapdata = mmap(NULL, PAGE_SIZE, PROT_WRITE | PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		end = rdtsc();
		mapdata[0]=0;
		act[i] = (int)(end-start);
		start = rdtsc();
		munmap(mapdata, PAGE_SIZE);
		end = rdtsc();
		deact[i] = (int)(end-start);
	}

	tot1 = tot2 = 0;
	qsort(act, ITER, sizeof(int), comp);
	qsort(deact, ITER, sizeof(int), comp);
	for(i=0; i<ITER; i++) {
		tot1 += (unsigned long long)act[i];
		tot2 += (unsigned long long)deact[i];
	}

	printf("map avg %llu %d %d %d\n", tot1/ITER, act[ITER/100], act[ITER/1000], act[ITER/10000]);
	printf("unmap avg %llu %d %d %d\n", tot2/ITER, deact[ITER/100], deact[ITER/1000], deact[ITER/10000]);

	return 0;
}
