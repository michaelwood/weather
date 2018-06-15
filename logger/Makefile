CLFAGS="-g -pedantic -Wall -Wextra -Wno-unused-parameter"

all: wmlog

wmlog: wmlog.c
	gcc $(CFLAGS) wmlog.c `pkg-config glib-2.0 libusb --libs --cflags` -o wmlog 

clean:
	rm wmlog

