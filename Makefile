CC=gcc
CFLAGS=-Wall
EXE=htproxy
OBJ=

all: $(EXE)

$(EXE): main.c $(OBJ)
	$(CC) -O3 $(CFLAGS) -o $(EXE) $< $(OBJ)

%.o: %.c %.h
	$(CC) -O3 $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(EXE)

format:
	clang-format -style=file -i *.c *.h
