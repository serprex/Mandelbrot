all: pitman taylor gabelsberger estoup stief
pitman: pitman.c
	gcc -s -std=gnu1x -fwhole-program -O3 -ffast-math -lGL -lX11 -pthread -march=native pitman.c -o pitman
taylor: taylor.c
	gcc -s -std=gnu1x -fwhole-program -O3 -ffast-math -lGL -lX11 -pthread -march=native taylor.c -o taylor
gabelsberger: gabelsberger.c
	gcc -s -std=gnu1x -fwhole-program -O3 -ffast-math -lGL -lX11 -lgmp -pthread -march=native gabelsberger.c -o gabelsberger
estoup: estoup.c
	gcc -s -std=gnu1x -fwhole-program -O3 -ffast-math -lGL -lX11 -lOpenCL -march=native estoup.c -o estoup
stief: stief.c
	gcc -s -std=gnu1x -fwhole-program -O3 -ffast-math -lGL -lGLEW -lX11 -march=native stief.c -o stief