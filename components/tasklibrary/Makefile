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

PROGRAM = tasklibrary
C_FILES := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(C_FILES))
CC = $(CROSS_COMPILE)gcc
CFLAGS = -D_GNU_SOURCE -Wall -pedantic -std=c99 -static -g
LDFLAGS = -lm

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

# These are the pattern matching rules. In addition to the automatic
# variables used here, the variable $* that matches whatever % stands for
# can be useful in special cases.
%.o: %.c timerload.h
	$(CC) $(CFLAGS) -c $< -o $@

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

install:
	@echo "Installing $(TOOLS_DIR)/$(PROGRAM)..."
	cp $(PROGRAM) $(TOOLS_DIR)

clean:
	@echo "Cleaning $(PROGRAM)..."
	rm -f $(PROGRAM) *.o tasklibrary
	@rm -f $(TOOLS_DIR)/$(PROGRAM)

.PHONY: clean install
