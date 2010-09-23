all: pitman gabelsberger ogabelsberger zgabelsberger
pitman: pitman.c
	gcc -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -lpthread -march=native pitman.c -o pitman
gabelsberger: gabelsberger.c
	gcc -g -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -lgmp -lpthread -march=native gabelsberger.c -o gabelsberger