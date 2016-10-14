#include <stdio.h>
#include <stdlib.h>
#include <gnu/libc-version.h>

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))
#define PAGE_SIZE 4096

int main()
{
#define total_loop_num 100000
#define inner_loop_num 20
	char *addrs[inner_loop_num];
	int sz, i, j;
	unsigned long long start, end, alloc_tot, free_tot;
	puts(gnu_get_libc_version());

	for(sz = PAGE_SIZE; sz<=1024*PAGE_SIZE; sz *= 2) {
		alloc_tot = free_tot = 0;
		for(j=0; j<inner_loop_num; j++) {
			addrs[j] = malloc(sz);
			/* assert(addrs[j]); */
			//addrs[j][0] = '$';
		}
		for(j=0; j<inner_loop_num; j++) {
			free(addrs[j]);
		}
		for(i=0; i<total_loop_num/inner_loop_num; i++) {
			rdtscll(start);
			for(j=0; j<inner_loop_num; j++) {
				addrs[j] = malloc(sz);
				/* assert(addrs[j]); */
				//addrs[j][0] = '$';
			}
			rdtscll(end);
			alloc_tot += (end-start);
			rdtscll(start);
			for(j=0; j<inner_loop_num; j++) {
				free(addrs[j]);
			}
			rdtscll(end);
			free_tot += (end-start);
		}
		printf("num pages %d libc malloc %llu\n", sz/PAGE_SIZE, alloc_tot/total_loop_num);
		printf("num pages %d libc free %llu\n", sz/PAGE_SIZE, free_tot/total_loop_num);
	}
	return 0;
}
