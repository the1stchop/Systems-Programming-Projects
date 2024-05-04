CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic
EXEC = memory
OBJS = memory.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS)

memory.o: memory.c
	$(CC) $(CFLAGS) -c memory.c

clean: 
	rm -f memory memory.o

format:
	clang-format -i -style=file *.[ch]
