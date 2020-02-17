# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#  -fno-stack-protector, to enforce the buffer overflow phenomenon.
#  -D_FORTIFY_SOURCE=0
CFLAGS  = -g -I.

# the build target executable:
DEPS = sockcom.h
TARGET1 = client
TARGET2 = server
TARGET3 = proxy

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:  $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(TARGET1).o
	$(CC) -o $@ $< $(CFLAGS) 

$(TARGET2): $(TARGET2).o
	$(CC) -o $@ $< $(CFLAGS) 

$(TARGET3): $(TARGET3).o
	$(CC) -o $@ $< $(CFLAGS) 

clean:
	$(RM) $(TARGET1) $(TARGET2) $(TARGET3) *.o

#DO NOT delete this line, make depends on this line
