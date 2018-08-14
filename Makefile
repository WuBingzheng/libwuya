CFLAGS = -g -O2

all: libwuya.a

libwuya.a: wuy_dict.o wuy_pool.o wuy_heap.o wuy_event.o wuy_sockaddr.o wuy_skiplist.o
	ar rcs $@ $^

wuy_dict.o: wuy_dict.c wuy_dict.h wuy_hlist.h

wuy_pool.o: wuy_pool.c wuy_pool.h wuy_hlist.h

wuy_heap.o: wuy_heap.c wuy_heap.h

wuy_event.o: wuy_event.c wuy_event.h

wuy_sockaddr.o: wuy_sockaddr.c wuy_sockaddr.h

wuy_skiplist.o: wuy_skiplist.c wuy_skiplist.h

clean:
	rm *.o
