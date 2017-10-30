/***
 * Copyright 2015 by Gabriel Parmer.  All rights reserved.
 * This file is dual licensed under the BSD 2 clause license.
 *
 * Authors: Gabriel Parmer, gparmer@gwu.edu, 2015
 */

#include <stdio.h>
#include <stdlib.h>

#include <ps_slab.h>

#define SMALLSZ 1
#define LARGESZ 8000

struct small {
	char x[SMALLSZ];
};

struct larger {
	char x[LARGESZ];
};

PS_SLAB_CREATE_DEF(s, sizeof(struct small))
PS_SLAB_CREATE(l, sizeof(struct larger), PS_PAGE_SIZE * 128)
PS_SLAB_CREATE(hextern, sizeof(struct larger), PS_PAGE_SIZE * 128)

#define ITER       (1024)
#define SMALLCHUNK 2
#define LARGECHUNK 32

/* These are meant to be disassembled and inspected, to validate inlining/optimization */
void *
disassemble_alloc()
{ return ps_slab_alloc_l(); }
void
disassemble_free(void *m)
{ ps_slab_free_l(m); }

void
mark(char *c, int sz, char val)
{
	int i;

	for (i = 0 ; i < sz ; i++) c[i] = val;
}

void
chk(char *c, int sz, char val)
{
	int i;

	for (i = 0 ; i < sz ; i++) assert(c[i] == val);
}

struct small  *s[ITER];
struct larger *l[ITER];

#define RB_SZ   (1024 * 32)
#define RB_ITER (RB_SZ * 1024)

void * volatile ring_buffer[RB_SZ] PS_ALIGNED;

unsigned long long free_tsc, alloc_tsc;

void
consumer(void)
{
	struct small *s;
	unsigned long i;
	unsigned long long start, end, tot = 0;

	meas_barrier(2);

	for (i = 0 ; i < RB_ITER ; i++) {
		unsigned long off = i % RB_SZ;

		while (!ring_buffer[off]) ;
		s = ring_buffer[off];
		ring_buffer[off] = NULL;

		start = ps_tsc();
		ps_slab_free_s(s);
		end = ps_tsc();
		tot += end-start;
	}
	free_tsc = tot / RB_ITER;
}

void
producer(void)
{
	struct small *s;
	unsigned long i;
	unsigned long long start, end, tot = 0;

	meas_barrier(2);

	for (i = 0 ; i < RB_ITER ; i++) {
		unsigned long off = i % RB_SZ;

		while (ring_buffer[off]) ;

		start = ps_tsc();
		s = ps_slab_alloc_s();
		end = ps_tsc();
		tot += end-start;

		assert(s);
		ring_buffer[off] = s;
	}
	alloc_tsc = tot / RB_ITER;
}

void *
child_fn(void *d)
{
	(void)d;

	thd_set_affinity(pthread_self(), 1);
	consumer();

	return NULL;
}

void
test_remote_frees(void)
{
	pthread_t child;

	printf("Starting test for remote frees\n");

	if (pthread_create(&child, 0, child_fn, NULL)) {
		perror("pthread create of child\n");
		exit(-1);
	}

	producer();

	pthread_join(child, NULL);
	printf("Remote allocations take %lld, remote frees %lld (unadjusted for tsc)\n", alloc_tsc, free_tsc);
}

void
test_correctness(void)
{
	int i, j;

	printf("Starting mark & check for increasing numbers of allocations.\n");
	for (i = 0 ; i < ITER ; i++) {
		l[i] = ps_slab_alloc_l();
		mark(l[i]->x, sizeof(struct larger), i);
		for (j = i+1 ; j < ITER ; j++) {
			l[j] = ps_slab_alloc_l();
			mark(l[j]->x, sizeof(struct larger), j);
		}
		for (j = i+1 ; j < ITER ; j++) {
			chk(l[j]->x, sizeof(struct larger), j);
			ps_slab_free_l(l[j]);
		}
	}
	for (i = 0 ; i < ITER ; i++) {
		assert(l[i]);
		chk(l[i]->x, sizeof(struct larger), i);
		ps_slab_free_l(l[i]);
	}
}

void
test_perf(void)
{
	int i, j;
	unsigned long long start, end;

	printf("Slabs:\n"
	       "\tsmall: objsz %lu, objmem %lu, nobj %lu\n"
	       "\tlarge: objsz %lu, objmem %lu, nobj %lu\n"
	       "\tlarge+nohead: objsz %lu, objmem %lu, nobj %lu\n",
	       (unsigned long)sizeof(struct small),  (unsigned long)ps_slab_objmem_s(), (unsigned long)ps_slab_nobjs_s(),
	       (unsigned long)sizeof(struct larger), (unsigned long)ps_slab_objmem_l(), (unsigned long)ps_slab_nobjs_l(),
	       (unsigned long)sizeof(struct larger), (unsigned long)ps_slab_objmem_hextern(), (unsigned long)ps_slab_nobjs_hextern());

	start = ps_tsc();
	for (j = 0 ; j < ITER ; j++) {
		for (i = 0 ; i < LARGECHUNK ; i++) s[i] = ps_slab_alloc_l();
		for (i = 0 ; i < LARGECHUNK ; i++) ps_slab_free_l(s[i]);
	}
	end = ps_tsc();
	end = (end-start)/(ITER*LARGECHUNK);
	printf("Average cost of large slab alloc+free: %lld\n", end);

	ps_slab_alloc_s();
	start = ps_tsc();
	for (j = 0 ; j < ITER ; j++) {
		for (i = 0 ; i < SMALLCHUNK ; i++) s[i] = ps_slab_alloc_s();
		for (i = 0 ; i < SMALLCHUNK ; i++) ps_slab_free_s(s[i]);
	}
	end = ps_tsc();
	end = (end-start)/(ITER*SMALLCHUNK);
	printf("Average cost of small slab alloc+free: %lld\n", end);

	ps_slab_alloc_hextern();
	start = ps_tsc();
	for (j = 0 ; j < ITER ; j++) {
		for (i = 0 ; i < LARGECHUNK ; i++) s[i] = ps_slab_alloc_hextern();
		for (i = 0 ; i < LARGECHUNK ; i++) ps_slab_free_hextern(s[i]);
	}
	end = ps_tsc();
	end = (end-start)/(ITER*LARGECHUNK);
	printf("Average cost of extern slab header, large slab alloc+free: %lld\n", end);
}

void
stats_print(struct ps_mem *m)
{
	struct ps_slab_stats s;
	int i;

	printf("Stats for slab @ %p\n", (void*)m);
	ps_slabptr_stats(m, &s);
	for (i = 0 ; i < PS_NUMCORES ; i++) {
		printf("\tcore %d, slabs %zd, partial slabs %zd, nfree %zd, nremote %zd\n",
		       i, s.percore[i].nslabs, s.percore[i].npartslabs, s.percore[i].nfree, s.percore[i].nremote);
	}
}

void memcpy_single_test(void);
void memcpy_mutli_test(void);
int
main(void)
{
	thd_set_affinity(pthread_self(), 0);
//	memcpy_single_test();
	memcpy_mutli_test();
/*
	test_perf();

	stats_print(&__ps_mem_l);
	stats_print(&__ps_mem_s);
	test_correctness();
	stats_print(&__ps_mem_l);
	test_remote_frees();
	stats_print(&__ps_mem_s);
*/
	return 0;
}

#define CPU_FREQ 2700000
int copys = 0, array_sz = 0;
static inline unsigned long long
printmem(long long int n)
{
	long long int c = (long long int)1024;
	if (n < c) printf("%lldB ", n);
	else if (n < c*c) printf("%lldK ", n/c);
	else printf("%lldM ", n/c/c);
	return n/c/c;
}
static inline unsigned long long
printtime(unsigned long long n)
{
	printf("%lldms ", n/(unsigned long long)CPU_FREQ);
	return n/(unsigned long long)CPU_FREQ;
}
void
memcpy_single(void)
{
	unsigned long long start, end;
	int i, *a, *b;
	long long int mem, ti;

	a = (int *)malloc(array_sz);
	b = (int *)malloc(array_sz);
	memset(a, -1, array_sz);
	memset(b, 0, array_sz);

	start = ps_tsc();
	for (i=0; i<copys; i++) {
		memcpy(b, a, array_sz);
	}
	end = ps_tsc();

	mem = printmem((long long int)copys * (long long int)array_sz);
	ti = printtime(end - start);
	printmem((long long int)array_sz);
	printf("%lld\n", mem*1000/ti);
	free(a);
	free(b);
}
void 
memcpy_single_test(void)
{
	int i, c[15] = {65536*16, 65536*16, 65536*16, 65536, 65536, 65536, 32768, 32768, 16384, 8192, 4096, 2048, 1024, 516};
	copys = 65536;
	array_sz = 4 *1024;
	for(i=0; i<14; i++) {
		copys = c[i];
		memcpy_single();		
		array_sz *= 2;
	}
}

volatile int send, done;
int *from, *to;
static inline int
notify(volatile int *a, int v, int loop)
{
	int i;
	while (*a != v && !done) {
		for(i=0; i<loop; i++) i++;
	}
	return i;
}
void *
child_test(void *d)
{
	(void)d;

	thd_set_affinity(pthread_self(), 1);
	while (!done) {
		notify(&send, 1, 1000);
		memset(from, 2, array_sz);
		send = 0;
	}

	return NULL;
}
void
memcpy_mutli(void)
{
	unsigned long long start, end, tot = 0;
	int i;
	long long int mem, ti;
	
	for (i=0; i<copys; i++) {
		notify(&send, 0, 1000);
		start = ps_tsc();
		memcpy(to, from, array_sz);
		end = ps_tsc();
		tot += (end - start);
		send = 1;
	}

	mem = printmem((long long int)copys * (long long int)array_sz);
	ti = printtime(tot);
	printmem((long long int)array_sz);
	printf("%lld\n", mem*1000/ti);
}
void 
memcpy_mutli_test(void)
{
	pthread_t child;
	int i, c[15] = {65536*16, 65536*16, 65536*16, 65536, 65536, 65536, 32768, 32768, 16384, 8192, 4096, 2048, 1024, 516};
	copys = 65536;
	array_sz = 4 *1024;
	for(i=0; i<14; i++) {
		send = 1;
		done = 0;
		from = (int *)malloc(array_sz+PS_CACHE_LINE);
		to = (int *)malloc(array_sz+PS_CACHE_LINE);
		memset(from, -1, array_sz);
		memset(to, 0, array_sz);
		if (pthread_create(&child, 0, child_test, NULL)) {
			perror("pthread create of child\n");
			exit(-1);
		}
		copys = c[i];
		memcpy_mutli();
		done = 1;
		pthread_join(child, NULL);
		free((void *)from);
		free((void *)to);
		array_sz *= 2;
	}
}
