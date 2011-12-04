all: fdlinecombine

fdlinecombine: *.c
		${CC} *.c -Wall -O2 -ggdb -o fdlinecombine

fdlinecombine_static: *.c
		musl-gcc -O2 *.c -o fdlinecombine_static
