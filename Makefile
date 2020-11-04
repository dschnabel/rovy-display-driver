INCLUDES= -I /opt/vc/include/IL -I /opt/vc/include -I /opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux/ -I/opt/vc/src/hello_pi/libs/ilclient 

CFLAGS=-O2 -DRASPBERRY_PI  -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM

OMXLIBS=-L /opt/vc/lib -Wl,--whole-archive -L/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -L/opt/vc/src/hello_pi/libs/ilclient -lilclient -L../libs/vgfont -Wl,--no-whole-archive -rdynamic

all: display test

display: display.c
	cc -shared $(CFLAGS) $(INCLUDES) -o libdisplay.so display.c $(OMXLIBS)

test: test.cpp
	cc test.cpp -o test -L. -ldisplay

clean:
	rm -f libdisplay.so test