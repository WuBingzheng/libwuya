CFLAGS = -g -O2 -I../
LDFLAGS = -L../
LDLIBS = -lwuya

all: dict_skiplist

dict_skiplist: dict_skiplist.o
	gcc -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f dict_skiplist *.o
