PSPSDK=$(shell psp-config --pspsdk-path)
PSPDIR=$(shell psp-config --psp-prefix)

TARGET = imglib_demo
OBJS = main.o callbacks.o graphics2d.o

CFLAGS = -O2 -G0 -Wall -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lstdc++ -lpspgum -lpspgu -lpng -lz -lm -lpsprtc
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = new img lib demo

include $(PSPSDK)/lib/build.mak

