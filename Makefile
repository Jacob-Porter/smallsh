CC=gcc
CFLAGS=-Wall -g 
DEPS = command.h list.h
OBJ = command.o list.o main.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

smallsh: $(OBJ)
	$(CC) --std=gnu99 -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o $(EXE_FILE)
	rm smallsh
