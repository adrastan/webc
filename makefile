INC=-I../include/
OBJ=obj/main.o

all: clean build run

build: $(OBJ)
	gcc -o out/main $(OBJ)

obj/%.o: src/%.c
	gcc $(INC) -c $< -o obj/$(@F)

clean:
	rm -f obj/* out/*

run:
	./out/main 
