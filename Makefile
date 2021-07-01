CC = gcc
OBJS = coord_traffic.o
CFLAGS = -Wall -pthread -c
LFLAGS = -Wall -pthread

coord_traffic: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o coord_traffic

coord_traffic.o:
	$(CC) $(CFLAGS) coord_traffic.c

clean:
	rm coord_traffic *.o *~
