#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#define PAGE_SIZE 4096
#define NUM_PROC 2
#define NUM_PAGE 1024
#define NUM_ITER 1000000
#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))
#define UHZ 2890

int pipes[NUM_PROC][2], pids[NUM_PROC];
unsigned long long int re[NUM_ITER];

static inline unsigned long long int
Max(void)
{
	int i;
	unsigned long long int s = re[0];
	for(i=1; i<NUM_ITER; i++) if (re[i]>s) s = re[i];
	return s;
}

static inline unsigned long long int
Min(void)
{
	int i;
	unsigned long long int s = re[0];
	for(i=1; i<NUM_ITER; i++) if (re[i]<s) s = re[i];
	return s;
}

static inline unsigned long long int
Avg(void)
{
	int i;
	unsigned long long int s = 0;
	for(i=0; i<NUM_ITER; i++) s += re[i];
	return s/NUM_ITER;
}

void read_write_pipe(int k)
{
//	char *buf;
char buf[1];
	int pid, i;
	pid = getpid();
//	buf = (char *)malloc(NUM_PAGE*PAGE_SIZE);
	//close unused pipes
	for(i=1; i<NUM_PROC; i++) {
		if (i == k-1) close(pipes[i][1]);
		else if (i == k) close(pipes[k][0]);
		else {
			close(pipes[i][0]);
			close(pipes[i][1]);
		}
	}
	while (1) {
/*		if (-1 == read(pipes[k-1][0], buf, NUM_PAGE*PAGE_SIZE) || buf[0] != '$') {
			printf("%d read error\n", pid);
		}
		buf[0] = '&';
		if (NUM_PAGE*PAGE_SIZE != write(pipes[k][1], buf, NUM_PAGE*PAGE_SIZE)) {
			printf("%d write error\n", pid);
		}*/
		if (-1 == read(pipes[k-1][0], buf, 1) || buf[0] != '$') {
			printf("%d read error\n", pid);
		}
		buf[0] = '&';
		if (1 != write(pipes[k][1], buf, 1)) {
			printf("%d write error\n", pid);
		}
	}
}

int main()
{
	unsigned long long int start = 0, end = 0, avg, stdev;
	int i, sz;
//	char *buf;
char buf[1];
	//create all pipes
	for(i=0; i<NUM_PROC; i++) {
		if (pipe (pipes[i])) {
			printf("create pipe error\n");
			return 0;
		}
	}
	//create all child process
	pids[0] = getpid();
	for(i=1; i<NUM_PROC; i++) {
		pids[i] = fork();
		switch (pids[i]) {
			case -1:
				printf("fork error\n");
				return 0;
			case 0:
				read_write_pipe(i);
			default:
				printf("crt proc %d\n", pids[i]);
				break;
		}
	}
	//close unused pipes
	for(i=1; i<NUM_PROC; i++) {
		if (i == NUM_PROC-1) close(pipes[i][1]);
		else if (i == 0) close(pipes[i][0]); 
		else {
			close(pipes[i][0]);
			close(pipes[i][1]);
		}
	}
//	buf = (char *)malloc(NUM_PAGE*PAGE_SIZE);
	for(i=0; i<NUM_ITER; i++) {
		rdtscll(start);
		buf[0] = '$';
		/*if (NUM_PAGE*PAGE_SIZE != write(pipes[0][1], buf, NUM_PAGE*PAGE_SIZE)) {
			printf("%d write error\n", pids[0]);
		}
		if (-1 == read(pipes[NUM_PROC-1][0], buf, NUM_PAGE*PAGE_SIZE) || buf[0] != '&') {	
			printf("%d read error\n", pids[0]);
		}*/
		if (1 != write(pipes[0][1], buf, 1)) {
			printf("%d write error\n", pids[0]);
		}
		if (-1 == read(pipes[NUM_PROC-1][0], buf, 1) || buf[0] != '&') {	
			printf("%d read error buf %c\n", pids[0], buf[0]);
		}
		rdtscll(end);
		assert(end>start);
		re[i] = end-start;
	}
	avg = Avg();
	printf("max %llu min %llu\n", Max(), Min());
	for(i=0; i<NUM_ITER; i++) re[i] = (re[i]-avg)*(re[i]-avg);
	stdev = sqrt(Avg());
	printf("%d proc %d page time %llu cyc %llu\n", NUM_PROC, NUM_PAGE, avg, stdev);
	return 0;
}
