CROSS_COMPILE=
CC	= $(CROSS_COMPILE)gcc
STRIP	= $(CROSS_COMPILE)strip
AR	= $(CROSS_COMPILE)ar

LIBPATH = 
INCLUDEPATH = include

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

CFLAGS  = -g -Wall -Wno-format-security -DNCURSES_OPAQUE=0 -I. -I$(INCLUDEPATH)
SRC_C   +=  $(wildcard src/*.c)
OBJS    +=  $(SRC_C:%.c=%.o)

TEST_SRC_C   +=  $(wildcard src/test/*.c) $(wildcard src/CUnit/*.c) 
TEST_OBJS    +=  $(TEST_SRC_C:%.c=%.o)
TEST_LIBS    +=  $(LIBS) -lcurses libpseudoTCP.a

all: pseudoTCP test
	
pseudoTCP: $(OBJS)
	$(AR) cr libpseudoTCP.a $(OBJS) 	
	
test: pseudoTCP $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) $(TEST_LIBS)
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
   
clean:
	rm -rf $(OBJS) test libpseudoTCP.a

	

