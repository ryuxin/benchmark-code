/*
 * Initial Clone from : https://github.com/phanikishoreg/scheduler-benchmarks
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define USEC_WAIT   100
#define ITERS       1000000

extern void set_prio (unsigned int);

static __inline__ unsigned long long
rdtsc (void)
{
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}

int
main(int argc, char **argv)
{
	int pipe1_fd[2], pipe2_fd[2];
	pid_t child;
	unsigned long long total, start, end;
	char ch = 'a';

	if (pipe(pipe1_fd) < 0) {
		perror("pipe");
		return -1;
	}

	if (pipe(pipe2_fd) < 0) {
		perror("pipe");
		return -1;
	}

//	set_prio(1);
	child = fork();
	if (child == 0) {
		int i;

		close(pipe1_fd[0]);
		close(pipe2_fd[1]);

		/* making sure read blocks for predictability */
		usleep(USEC_WAIT); 

		for (i = 0 ; i < ITERS ; i ++) {
			start = rdtsc();
ch = 'a';
			if (write(pipe1_fd[1], &ch, 1) < 0) {
				perror("write");
				/* TODO: kill child! */
				_exit(-1);
			}
			if (read(pipe2_fd[0], &ch, 1) < 0) {
				perror("write");
				/* TODO: kill child! */
				_exit(-1);
			}
			end = rdtsc();

			total += (end - start);

		}
		close(pipe1_fd[1]);
		close(pipe2_fd[0]);

		printf("rpc - %llu\n", (total/(unsigned long long)ITERS));
		_exit(0);
	} else if (child > 0) {
		int i;

//		set_prio(0);
		close(pipe1_fd[1]);
		close(pipe2_fd[0]);

		for (i = 0 ; i < ITERS ; i ++) {
			if (read(pipe1_fd[0], &ch, 1) < 0) {
				perror("read");
				exit(-1);
			}
ch = 'a';
			if (write(pipe2_fd[1], &ch, 1) < 0) {
				perror("write");
				/* TODO: kill child! */
				exit(-1);
			}
		}

		close (pipe1_fd[0]);
		close (pipe2_fd[1]);

		wait(NULL);
		exit(0);
	} else {
		perror("fork");
	}

	close(pipe1_fd[0]);
	close(pipe1_fd[1]);
	close(pipe2_fd[0]);
	close(pipe2_fd[1]);

	return -1;
}
