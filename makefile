amp: main.c
	gcc -std=c11 -o play main.c -framework OpenGL -framework GLUT -L/usr/local/lib -lportaudio -lsndfile
