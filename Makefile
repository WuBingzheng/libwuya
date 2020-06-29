CFLAGS += -g -Wall -O2

all: libwuya.a

libwuya.a: wuy_dict.o wuy_heap.o wuy_event.o wuy_sockaddr.o wuy_skiplist.o \
	wuy_murmurhash.o wuy_cflua.o wuy_http.o
	ar rcs $@ $^

clean:
	rm -f *.o libwuya.a
