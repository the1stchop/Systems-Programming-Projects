CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic
EXEC = hello
OBJS = hello.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS)

hello.o: hello.c
	$(CC) $(CFLAGS) -c hello.c

clean: 
	rm -f hello hello.o

format:
	clang-format -i -style=file *.[ch]
