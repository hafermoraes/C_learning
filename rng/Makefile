P=rng
OBJECTS=
CFLAGS=`pkg-config --cflags gsl` -g -Wall -std=gnu99 -O0
LDLIBS=`pkg-config --libs gsl`
CC=gcc

$(P): $(OBJECTS)

clean:
	rm $(P)
