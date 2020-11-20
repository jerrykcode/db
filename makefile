TARGET = db
OBJS = disk.o table.o util.o datatype.o

CC = gcc

$(TARGET) : $(OBJS) main.o
	$(CC) -o $@ $^

clean :
	rm $(TARGET) $(OBJS) main.o

#test

test : run_test_disk
	rm $(OBJS)
	

test_disk : $(OBJS) test_disk.o
	$(CC) -o $@ $^
test_disk.o : test/test_disk.c
	$(CC) -c -o $@ $<

run_test_disk : test_disk test/test_disk.file
	./$<
	-diff file test/test_disk.file
	rm file
	rm test_disk.o
	rm $<

test_table : $(OBJS) test_table.o
	$(CC) -o $@ $^
test_table.o : test/test_table.c
	$(CC) -c -o $@ $<
