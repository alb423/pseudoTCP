CROSS_COMPILE=
CC	= $(CROSS_COMPILE)gcc
STRIP	= $(CROSS_COMPILE)strip
AR	= $(CROSS_COMPILE)ar

LIBPATH = 
INCLUDEPATH = -I. -Iinc -Itest/inc

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
   # For Linux (Ubuntu)
   LD_LIBRARY_PATH  = /usr/local/lib
   LIBS    = -lpthread -lrt  -rdynamic
endif
   
ifeq ($(UNAME_S),Darwin)
   # For MacOS
   LD_LIBRARY_PATH  = /usr/local/lib
   LIBS    = -L/opt/local/lib -lpthread -rdynamic
endif

CFLAGS  = -g -Wall -Wno-format-security $(INCLUDEPATH)
SRC_C   +=  $(wildcard src/*.c)
OBJS    +=  $(SRC_C:%.c=%.o)

TEST_SRC_C   +=  $(wildcard src/*.c) $(wildcard test/*.c) $(wildcard test/CUnit/*.c) $(wildcard test/pseudotcp/*.c)
TEST_OBJS    +=  $(TEST_SRC_C:%.c=%.o)
TEST_LIBS    +=  $(LIBS) libpseudoTCP.a

all: pseudoTCP pseudoTCP_test
	
pseudoTCP: $(OBJS)
	$(AR) cr libpseudoTCP.a $(OBJS) 	
	
pseudoTCP_test: pseudoTCP $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) $(TEST_LIBS)
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
   
clean:
	rm -rf $(OBJS) $(TEST_OBJS) pseudoTCP_test libpseudoTCP.a

	

