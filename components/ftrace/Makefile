################################################################################
# Copyright (C) 2015 ARM Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

#
# ftrace - framework and library that will analyse ftrace
#
# "make all" create the shared libraries and the main ftrace
# this is intented to run on host
#
# "make android" create a static ftrace_android with all libraries
# embeded this is intented to run on target.
#
# For cross compile uou must set CROSS_COMPILE,
# For example "CROSS_COMPILE=arm-linux-gnueabihf- make android" if you
# want to use arm-linux-gnueabihf-g++/arm-linux-gnueabihf-gcc
#
#
# If you want to add any new libraries, you just need to name it
# ftrace_so_NAME.c the makefile will automatically create
# the library that will be called using libNAME.so.1.0
#

PROGRAM=ftrace
HEADERS=ftrace_dl.h ftrace_decode.h ftrace_decode_internal.h ftrace_decode_binary.h ftrace_helper.h
SOURCES=ftrace_helper.c ftrace_dl.c ftrace.c
SOURCESCPP=ftrace_decode_binary.cpp ftrace_decode_ascii.cpp ftrace_decode.cpp
LIBFLAGS=-rdynamic -O4 -Wall -g -fPIC -shared -DFTRACE_DL_SO
CFLAGS=-rdynamic -O4 -Wall -ldl -pthread -g -lm -DFTRACE_DL_SO
ANDROID_CFLAGS= -O4 -Wall -lrt -ldl -pthread -g -lm -static

libsources := $(wildcard ftrace_so_*.c)
lib_object_android := $(patsubst ftrace_so_%.c,ftrace_so_%.o,$(libsources))
libexe := $(patsubst ftrace_so_%.c,lib%.so.1.0.0,$(libsources))

lib%.so.1.0.0: ftrace_so_%.c ftrace_dl.h ftrace_helper.h
	$(CROSS_COMPILE)gcc $(LIBFLAGS) $<  -Wl,-soname,$(patsubst ftrace_so_%.c,lib%.so.1,$<) -o $@

ftrace_so_%.o: ftrace_so_%.c ftrace_dl.h ftrace_helper.h
	$(CROSS_COMPILE)g++ $(ANDROID_CFLAGS) $< -DFTRACE_SO_LIBNAME=\"$(patsubst ftrace_so_%.c,lib%.so.1.0.0,$<)\" -c

ftrace_android: $(SOURCES) $(HEADERS) $(SOURCESCPP) $(lib_object_android)
	$(CROSS_COMPILE)g++ $(lib_object_android) $(SOURCESCPP) $(SOURCES) $(ANDROID_CFLAGS)  -I.  -o $@

ftrace: $(SOURCES) $(HEADERS)  $(SOURCESCPP)
	g++ $(SOURCESCPP) $(SOURCES) $(CFLAGS) -I. -o $@

all: ftrace $(libexe)

android: ftrace_android

install:
	@echo "Installing $(TOOLS_DIR)/$(PROGRAM)..."
	cp ftrace_android $(TOOLS_DIR)/$(PROGRAM)

clean:
	@echo "Cleaning $(PROGRAM)..."
	@rm -f ftrace_android ftrace $(libexe) $(lib_object_android)
	@rm -f $(TOOLS_DIR)/$(PROGRAM)

.PHONY: install clean

