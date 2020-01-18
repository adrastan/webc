INC=-Iinclude/ -I/usr/include/glib-2.0/
LIB=-L/usr/lib
OBJ=obj/main.o obj/sock.o obj/message.o obj/uri.o obj/server.o

all: clean build 

build: $(OBJ)
	gcc $(LIB) -o out/main $(OBJ) -lpthread -lm -lglib-2.0

obj/%.o: src/%.c
	gcc $(INC) -c $< `pkg-config --cflags --libs glib-2.0` -o obj/$(@F)

clean:
	rm -f obj/* out/main

run:
	./out/main 
