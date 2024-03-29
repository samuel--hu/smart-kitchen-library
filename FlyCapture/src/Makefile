OBJS=$(patsubst %.cpp,%.o,$(shell find ./ -name "*.cpp" -print))

LIBNAME = libskl_flycapture

ADDITIONALMODULE = OpenCV Core


L_BITS := $(shell getconf LONG_BIT)

CC = g++
CFLAGS = -Wall
CFLAGS_D = -Wall

# options for OpenCV
piriod:= .
empty:=
space:= $(empty) $(empty)
OPENCVVER	= $(subst $(piriod),$(space),$(shell pkg-config --modversion opencv))
INC += `pkg-config --cflags opencv`
LIBS_OPT += `pkg-config --libs opencv`

CFLAGS += -DCV_MAJOR_VERSION=$(word 1,$(OPENCVVER))
CFLAGS += -DCV_MINOR_VERSION=$(word 2,$(OPENCVVER))
CFLAGS += -DHAVE_TBB
CFLAGS_D += -DCV_MAJOR_VERSION=$(word 1,$(OPENCVVER))
CFLAGS_D += -DCV_MINOR_VERSION=$(word 2,$(OPENCVVER))
CFLAGS_D += -DHAVE_TBB

INC += -I../include -I../../include
INC += ${ADDITIONALMODULE:%=-I../../%/include/}

# FlyCapture$B$NFI$_9~$_(B
INC += -I/usr/include/flycapture/
LIBS += -lflycapture
LIBS_DEBUG += -lflycapture


.SUFFIXES:.cpp

.PHONY:clean debug

default:${LIBNAME}.a

debug:${LIBNAME}_d.a

${LIBNAME}.a:${OBJS:%=${LIBNAME}.a(%)}

${LIBNAME}(%.o):%.o

${LIBNAME}_d.a:${OBJS:%.o=${LIBNAME}_d.a(%_d.o)}

${LIBNAME}_d(%_d.o):%_d.o

# hoge.cpp -> hoge.o
.cpp.o:
	$(CC) -O2 -c $(CFLAGS) -o $@ $< $(INC) -D__linux__

# hoge.cpp -> hoge_d.o
%_d.o:%.cpp
	$(CC) -g -c $(CFLAGS) -o $@ $< $(INC) -D_DEBUG -D__linux__

clean:
	rm -f *.o
	rm -f $(LIBNAME).a
	rm -f $(LIBNAME)_d.a
