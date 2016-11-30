CC = gcc
#Using -Ofast instead of -O3 might result in faster code,
#but is supported only by newer GCC versions
CFLAGS = -lm -pthread -O3 -march=native -Wall -funroll-loops \
				 -Wno-unused-value
DBGFLAGS = -lm -pthread -O0 -g -v -da -Q

all 	: s3e test

s3e 	: s3e.c 
	$(CC) s3e.c -o s3e $(CFLAGS)
test 	: s3e_test.c s3e.c text_proc.c misc.c
	$(CC) s3e_test.c -o s3e_test $(CFLAGS)
dbg 	: s3e.c
	$(CC) s3e.c -o s3e $(DBGFLAGS)
test2 	: test.c s3e.c
	$(CC) test.c -o test $(CFLAGS)
clean:
	rm -rf s3e s3e_test test
