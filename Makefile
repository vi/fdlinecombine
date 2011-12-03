all: fdlinecombine

fdlinecombine: *.c
		${CC} *.c -ggdb -o fdlinecombine

fdlinecombine_static: *.c
		musl-gcc -O2 *.c -o fdlinecombine_static
