TARGET = db
OBJS = disk.o main.o

CC = gcc

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS)

clean :
	rm $(TARGET) $(OBJS)
