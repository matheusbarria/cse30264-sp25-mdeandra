# Makefile

CC=g++ -std=c++11
CFLAGS=

LD=g++
LDFLAGS=
LIBS=

SOURCE=	$(wildcard *.cc *.c)
OBJECTS=	$(SOURCE:.cc=.o)
TARGETS=	tracker

all: $(TARGETS)			# Default target

tracker: 	$(OBJECTS)		# Executable target
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o:		%.c		# Object targets
	$(CC) -c $(CFLAGS) -o $@ $^

clean:				# Clean target
	rm -f $(TARGETS) *.o
