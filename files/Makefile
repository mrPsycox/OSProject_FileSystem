CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes -lstdc++
LIBS=
CC=gcc
AR=ar

HEADERS=bitmap.h\
	disk_driver.h\
	simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS)

so_game: simplefs_test.c $(OBJS)
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

test_functions: simplefs_test.c bitmap.c disk_driver.c simplefs.c $(HEADERS)
								$(CC) $(CCOPTS) simplefs_test.c bitmap.c disk_driver.c simplefs.c -o test_functions

fs: file_system.c bitmap.c disk_driver.c simplefs.c $(HEADERS)
		$(CC) $(CCOPTS) file_system.c bitmap.c disk_driver.c simplefs.c -o fs

clean:
	rm -rf *.o *~  fs disk.txt
