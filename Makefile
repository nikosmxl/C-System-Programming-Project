CC = gcc
TARGET = mysh

all:
	$(CC) -o $(TARGET) myshell.c redirection.c myglob.c alias.c history.c

clean:
	rm $(TARGET)