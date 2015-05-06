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

# Detect Android userland
ANDROID_PATH=/system/build.prop
if [ -f $ANDROID_PATH ]; then
	ANDROID=1
	SHELL_CMD=sh
else
	ANDROID=0
	SHELL_CMD=bash
fi

function pidkiller()
{
  if [ $ANDROID -eq 0 ]; then
    disown $1
  fi
  kill -9 $1 >/dev/null 2>&1
}
