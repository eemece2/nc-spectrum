split: nc-spectrum.c
	gcc nc-spectrum.c -o nc-spectrum -lncurses `pkg-config --cflags --libs gstreamer-0.10`
