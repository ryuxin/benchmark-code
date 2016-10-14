CC := gcc
RM := rm
CFLAGS := -O3 -g

all: pipe_ipc pipe_rpc threads mmap

%: %.c priority.c
	$(CC) $(CFLAGS) $< priority.c -o $@.out -lrt -lpthread

clean:
	$(RM) -f *.o *.out 
