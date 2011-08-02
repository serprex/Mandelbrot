all: pitman gabelsberger estoup stief
pitman: pitman.c
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -pthread -march=native pitman.c -o pitman
gabelsberger: gabelsberger.c
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -lgmp -pthread -march=native gabelsberger.c -o gabelsberger
estoup: estoup.c
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGL -lOpenCL -march=native estoup.c -o estoup
stief: stief.c
	gcc -s -std=gnu99 -fwhole-program -O3 -ffast-math -lGLEW -march=native stief.c -o stief