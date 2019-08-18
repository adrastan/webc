INC=-Iinclude/
OBJ=obj/main.o obj/sock.o

all: clean build 

build: $(OBJ)
	gcc -o out/main $(OBJ) -lpthread

obj/%.o: src/%.c
	gcc $(INC) -c $< -o obj/$(@F)

clean:
	rm -f obj/* out/*

run:
	./out/main 
