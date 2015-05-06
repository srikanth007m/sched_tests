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

PROGRAM=regression
SRC:=chew.c  fiftyp.c  hackbench.c  irman2.c  massive_intr.c  onetwotest.c  starve.c  tenp.c \
 test.c  testcase.c  test.h  thud2.c  thud.c migration.c pipe-test.c pipe-test-1m.c cpuhog.c \
pth_str01.c pth_str02.c pth_str03.c

regression: $(SRC)
	$(CROSS_COMPILE)gcc $(SRC) -lrt -g -pthread -lm -o $@ -static

all: regression

android: regression

install:
	@echo "Installing $(TOOLS_DIR)/$(PROGRAM)..."
	cp $(PROGRAM) $(TOOLS_DIR)

clean:
	@echo "Cleaning $(PROGRAM)..."
	@rm -f $(PROGRAM)
	@rm -f $(TOOLS_DIR)/$(PROGRAM)

.PHONY: install clean

