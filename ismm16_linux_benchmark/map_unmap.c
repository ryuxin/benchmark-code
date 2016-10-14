#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <gnu/libc-version.h>

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))
#define PAGE_SIZE 4096
#define NUM_ITER 100000
unsigned long long int a_re[NUM_ITER], f_re[NUM_ITER];

static inline unsigned long long int
Max(unsigned long long int *re)
{
	int i;
	unsigned long long int s = re[0];
	for(i=1; i<NUM_ITER; i++) if (re[i]>s) s = re[i];
	return s;
}

static inline unsigned long long int
Min(unsigned long long int *re)
{
	int i;
	unsigned long long int s = re[0];
	for(i=1; i<NUM_ITER; i++) if (re[i]<s) s = re[i];
	return s;
}

static inline unsigned long long int
Avg(unsigned long long int *re)
{
	int i;
	unsigned long long int s = 0;
	for(i=0; i<NUM_ITER; i++) s += re[i];
	return s/NUM_ITER;
}

int main()
{
#define total_loop_num 100000
#define inner_loop_num 20
	char *addrs[inner_loop_num];
	int sz, i, j=0;
	unsigned long long start, end, alloc_tot, free_tot;
	puts(gnu_get_libc_version());

	for(sz = PAGE_SIZE; sz<=1024*PAGE_SIZE; sz *= 2) {
		addrs[j] = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		memset(addrs[j], 0, sz);
		munmap(addrs[j], sz);
		for(i=0; i<total_loop_num; i++) {
			rdtscll(start);
			addrs[j] = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			rdtscll(end);
			a_re[i] = end-start;
	
//			addrs[j][0] = '$';
			memset(addrs[j], 0, sz);

			rdtscll(start);
			munmap(addrs[j], sz);
			rdtscll(end);
			f_re[i] = end-start;
		}
		printf("num pages %d libc alloc max %llu min %llu avg %llu\n", sz/PAGE_SIZE, Max(a_re), Min(a_re), Avg(a_re));
		printf("num pages %d libc free max %llu min %llu avg %llu\n", sz/PAGE_SIZE, Max(f_re), Min(f_re), Avg(f_re));
	}
	return 0;
}
