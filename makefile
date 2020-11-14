TARGET = db
OBJS = disk.o

CC = gcc

$(TARGET) : $(OBJS) main.o
	$(CC) -o $@ $(OBJS) main.o

clean :
	rm $(TARGET) $(OBJS) main.o

#test

test : run_test_disk
	rm $(OBJS)
	

test_disk : $(OBJS) test_disk.o
	$(CC) -o $@ $(OBJS) test_disk.o
test_disk.o : test/test_disk.c
	$(CC) -c -o $@ $<

run_test_disk : test_disk test/test_disk.file
	./$<
	-diff file test/test_disk.file
	rm file
	rm test_disk.o
	rm $<
