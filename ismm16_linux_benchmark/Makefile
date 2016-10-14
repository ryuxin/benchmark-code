all:
	gcc -O3 -o pipe_test micro_pipe.c -lm
	gcc -O3 -o libc_malloc micro_malloc.c
	gcc -O3 -o tc_malloc micro_malloc.c -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -ltcmalloc
	gcc -O3 -o stream_malloc micro_malloc.c ./libstreamflow.so
	gcc -O3 -o je_malloc micro_malloc.c /usr/local/lib/libjemalloc.a -pthread
	gcc -O3 -o check_libc_malloc micro_malloc_2.c
	gcc -O3 -o check_tc_malloc micro_malloc_2.c -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -ltcmalloc
	gcc -O3 -o check_stream_malloc micro_malloc_2.c ./libstreamflow.so
	gcc -O3 -o check_je_malloc micro_malloc_2.c /usr/local/lib/libjemalloc.a -pthread
	gcc -O3 -o map_unmap map_unmap.c

clean:
	rm -f pipe_test libc_malloc tc_malloc stream_malloc je_malloc check_libc_malloc check_tc_malloc check_stream_malloc check_je_malloc map_unmap
