#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define ITERS 1000000

extern void pthread_prio(pthread_t pid, unsigned int nice);
unsigned long long start, end, total;
pthread_t thd;
int testing;

static __inline__ unsigned long long
rdtsc(void)
{
	unsigned long long int x;
	__asm__ volatile ("rdtsc" : "=A" (x));
	return x;
}

void *
thd_fn(void *arg)
{
	pthread_yield();

	while (testing) {
		pthread_yield();
	}

	pthread_exit(NULL);
}

int
main(void)
{
	int i;

	set_prio(0);

	pthread_create(&thd, NULL, thd_fn, NULL);
	testing = 1;
	pthread_prio(thd, 0);

	for (i = 0 ; i < ITERS ; i ++) {
		start = rdtsc();
		pthread_yield();
		end = rdtsc();
		total += (end - start);
	}
	testing = 0;
	pthread_yield();

	pthread_join(thd, NULL);
	printf("%llu\n", total / (2 * ITERS));

	return 0;
}
