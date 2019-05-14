# Makefile for moonroot

CFLAGS = -g
LDFLAGS = -L/usr/X11R6/lib -lXpm -lXext -lX11 -lm

SRCS = moonroot.c mooncalcs.c
OBJS = $(subst .c,.o,$(SRCS))

all: moonroot

moonroot: $(OBJS)
	$(CC) -o moonroot $(OBJS) $(LDFLAGS)

clean:
	-rm -f *.[oas] *.ld core moonroot

