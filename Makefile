.PHONY:clean
GG=gcc
CFLAGS=-DDEBUG -Wall -g
BIN=minihttp
OBJS=main.o common.o sysutil.o
LIBS=

$(BIN):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(BIN)
   
   
