#
# gp2-c Makefile
#
# GNU Make required
#

# Determine architecture.
COMPILE_ARCH=$(shell uname -m | sed -e 's/i.86/x86/' | sed -e 's/^arm.*/arm/')

ifeq ($(COMPILE_ARCH),i86pc)
  COMPILE_ARCH=x86
endif

ifeq ($(COMPILE_ARCH),amd64)
  COMPILE_ARCH=x86_64
endif
ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=x86_64
endif

# Set build directory if not yet set.
ifndef BUILD_DIR
BUILD_DIR=build
endif

# Set optimization level.
OPTIMIZE=-O3

# Set base compiler info.
ifndef CC
  CC=gcc
endif

ifndef CXX
  CXX=g++
endif

# Set object file directory.
ODIR=obj

# Set output file names.
ifndef OUTPUT_C
	OUTPUT_C=gp2-test-c
endif
ifndef OUTPUT_CPP
	OUTPUT_CPP=gp2-test-cpp
endif

# Set C preprocessor definitions.
CFLAGS=$(OPTIMIZE) -DARCH_STRING=\"$(COMPILE_ARCH)\" -DDEDICATED

# Set linker flags.
LDFLAGS=-lm

# Set the mkdir command.
MKDIR=mkdir

# Set input files.
# Files for the C test.
C_DEPS = gp2-c/genericparser2.h \
  qcommon/q_platform.h \
  qcommon/q_shared.h \
  qcommon/qcommon.h \
  qcommon/surfaceflags.h \
  qcommon/tags.h \
  test.h

_C_OBJ = c/gp2-c/genericparser2.o \
  c/qcommon/common.o \
  c/qcommon/q_math.o \
  c/qcommon/q_shared.o \
  c/test.o \
  c/main.o
C_OBJ = $(patsubst %,$(ODIR)/%,$(_C_OBJ))

# Files for the C++ test.
CPP_DEPS = gp2-cpp/genericparser2.h \
  qcommon/q_platform.h \
  qcommon/q_shared.h \
  qcommon/qcommon.h \
  qcommon/surfaceflags.h \
  qcommon/tags.h \
  test.h

_CPP_OBJ = cpp/gp2-cpp/genericparser2.o \
  cpp/qcommon/common.o \
  cpp/qcommon/q_math.o \
  cpp/qcommon/q_shared.o \
  cpp/test.o \
  cpp/main.o
CPP_OBJ = $(patsubst %,$(ODIR)/%,$(_CPP_OBJ))

# ========
# Targets.
# ========

all: gp2-test-c gp2-test-cpp

$(ODIR)/c/%.o: %.c $(C_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(ODIR)/cpp/%.o: %.cpp $(CPP_DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)
$(ODIR)/cpp/%.o: %.c $(CPP_DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)

makedirs_c:
	@if [ ! -d $(ODIR) ];then $(MKDIR) $(ODIR);fi
	@if [ ! -d $(ODIR)/c ];then $(MKDIR) $(ODIR)/c;fi
	@if [ ! -d $(ODIR)/c/gp2-c ];then $(MKDIR) $(ODIR)/c/gp2-c;fi
	@if [ ! -d $(ODIR)/c/qcommon ];then $(MKDIR) $(ODIR)/c/qcommon;fi
makedirs_cpp:
	@if [ ! -d $(ODIR) ];then $(MKDIR) $(ODIR);fi
	@if [ ! -d $(ODIR)/cpp ];then $(MKDIR) $(ODIR)/cpp;fi
	@if [ ! -d $(ODIR)/cpp/gp2-cpp ];then $(MKDIR) $(ODIR)/cpp/gp2-cpp;fi
	@if [ ! -d $(ODIR)/cpp/qcommon ];then $(MKDIR) $(ODIR)/cpp/qcommon;fi

build_c: $(C_OBJ)
	$(CC) -o $(OUTPUT_C) $^ $(LDFLAGS)
gp2-test-c: makedirs_c build_c

build_cpp: $(CPP_OBJ)
	$(CXX) -o $(OUTPUT_CPP) $^ $(LDFLAGS)
gp2-test-cpp: makedirs_cpp build_cpp

.PHONY: clean

clean:
	rm -f $(ODIR)/c/*.o *~ core $(INCDIR)/*~
	rm -f $(ODIR)/c/gp2-c/*.o *~ core $(INCDIR)/*~
	rm -f $(ODIR)/c/qcommon/*.o *~ core $(INCDIR)/*~
	rm -f $(ODIR)/cpp/*.o *~ core $(INCDIR)/*~
	rm -f $(ODIR)/cpp/gp2-cpp/*.o *~ core $(INCDIR)/*~
	rm -f $(ODIR)/cpp/qcommon/*.o *~ core $(INCDIR)/*~	
