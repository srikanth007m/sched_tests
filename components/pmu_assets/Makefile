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

CC = $(CROSS_COMPILE)gcc
CFLAGS = -O0 -ggdb3 -Wall -static
EXES = cycles cache branch
SRCS = $(EXES:=.c)
OBJS = $(EXES:=.o)

all: $(EXES)

$(EXES): $(SRCS)
	$(CC) $(CFLAGS) $@.c -o $@

install:
	@echo "Installing $(TOOLS_DIR)/{$(EXES)}..."
	$(foreach exe,$(EXES),cp $(exe) $(TOOLS_DIR);)

clean:
	@echo "Cleaning $(PROGRAM)..."
	rm -f $(EXES) $(OBJS)
	$(foreach exe,$(EXES),rm -f $(TOOLS_DIR)/$(exe);)

.PHONY:	install clean
