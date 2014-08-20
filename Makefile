CROSS_COMPILE=
CC	= $(CROSS_COMPILE)gcc
STRIP	= $(CROSS_COMPILE)strip

LIBPATH = 
INCLUDEPATH =

#CFLAGS  = -c -Wall -Wno-format-security 
#INCLUDEPATH = /usr/include/CUnit


UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
   # For Linux (Ubuntu)
   LD_LIBRARY_PATH  = /usr/local/lib
   INCLUDEPATH =
   LIBS    = -lpthread -lrt -lcunit -rdynamic
endif
   
ifeq ($(UNAME_S),Darwin)

   # For MacOS
   # -I/opt/local/include/ -L/opt/local/lib
   LD_LIBRARY_PATH  = /usr/local/lib /opt/local/lib
   INCLUDEPATH = /opt/local/include/
   LIBS    = -L/opt/local/lib -lpthread -lcunit -rdynamic
endif

CFLAGS  = -g -Wall -Wno-format-security -I. -I$(INCLUDEPATH)
OBJ     = pseudo_tcp.o mi_util.o fifo_buffer.o segment_list.o pseudo_tcp_porting.o \
test/main.o test/pseudo_tcp_unittest.o test/fifo_buffer_unittest.o test/segment_list_unittest.o \
test/memory_stream.o test/message_queue.o test/memory_stream_unittest.o 

SOURCES = pseudo_tcp.c mi_util.c fifo_buffer.c segment_list.c pseudo_tcp_porting.c \
test/main.c test/pseudo_tcp_unittest.c test/fifo_buffer_unittest.c test/segment_list_unittest.c \
test/memory_stream.c test/message_queue.c test/memory_stream_unittest.c


all: pseudoTCP

pseudoTCP: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)
# remember remove main() function of xtables_multi-xtables-multi.o
	#rm /home/albert/iptables/iptables/xtables_multi-xtables-multi.o
	#libtool --mode=link gcc -g -O -o pseudoTCP pseudo_tcp.o mi_util.o
   
clean:
	rm -rf *~ *.o test/*.o pseudoTCP

	

