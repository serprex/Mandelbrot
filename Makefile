all: pitman gabelsberger estoupe
pitman: pitman.c
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -pthread -march=native pitman.c -o pitman
gabelsberger: gabelsberger.c
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -lgmp -pthread -march=native gabelsberger.c -o gabelsberger
estoupe: estoupe.c estoupe.cl
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -lOpenCL -march=native estoupe.c -o estoupe