TARGET = db
OBJS = disk.o table.o util.o datatype.o rbtree.o stack.o map.o btree.o

CC = gcc

DEBUG_FLAG = -g
CFLAGS = $(DEBUG_FLAG)

%.o : %.c
	$(CC) $< $(CFLAGS) -c -o $@

$(TARGET) : $(OBJS) main.o
	$(CC) $(CFLAGS) -o $@ $^

clean :
	rm $(TARGET) $(OBJS) main.o

#test

test : run_test_disk run_test_map run_test_table
	rm $(OBJS)
	rm *.frm
	rm *.dat
	rm *.idx

# test disk
test_disk : $(OBJS) test_disk.o
	$(CC) $(CFLAGS) -o $@ $^
test_disk.o : test/test_disk.c
	$(CC) $< $(CFLAGS) -c -o $@

run_test_disk : test_disk test/test_disk.file
	./$<
	-diff file test/test_disk.file
	rm file
	rm test_disk.o
	rm $<

# test map
test_map: map.o rbtree.o stack.o test_map.o
	$(CC) $(CFLAGS) -o $@ $^
test_map.o : test/test_map.c
	$(CC) $< $(CFLAGS) -c -o $@

run_test_map : test_map
	./$<
	rm test_map.o
	rm $<

# test table
test_table : $(OBJS) test_table.o
	$(CC) $(CFLAGS) -o $@ $^
test_table.o : test/test_table.c
	$(CC) $< $(CFLAGS) -c -o $@

run_test_table : test_table test/test_table.file
	./$< > file	
	-diff file test/test_table.file
	rm file
	rm test_table.o
	rm $<
