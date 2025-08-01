CC=gcc
CFLAGS=-Wall -g
EXE=tcp_sim
OBJ=launcher.o app_layer.o tcp_layer.o

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -O3 $(CFLAGS) -o $(EXE) $(OBJ)

%.o: %.c
	$(CC) -O3 $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(EXE)

format:
	clang-format -style=file -i *.c *.h
