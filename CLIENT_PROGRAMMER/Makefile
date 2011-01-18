CC=gcc
CFLAGS=
DEPS = programmer.h
OBJ = programmer.o nandlib.o sernetlib.o seriallib.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

programmer: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
