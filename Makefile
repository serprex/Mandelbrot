all: pitman.c
	gcc -std=gnu99 -fwhole-program -O3 -ffast-math pitman.c -o pitman -lX11 -lGL -lpthread -march=native
