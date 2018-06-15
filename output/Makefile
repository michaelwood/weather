all: wmlog-parser

wmlog-parser: wmlog-parser.c
	gcc -Wall -lm `pkg-config glib-2.0 --cflags --libs` -g wmlog-parser.c -o csv
