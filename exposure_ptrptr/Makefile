
P=exposure
OBJECTS=
CFLAGS=`pkg-config --cflags glib-2.0` -g -Wall -Werror -ansi -pedantic -std=gnu99 -O0
LDLIBS=`pkg-config --libs glib-2.0`
CC=gcc

$(P): $(OBJECTS)

clean:
	rm $(P)
