#!/bin/bash
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

NOW=`date +%Y%m%d_%H%M%S`
export OUTDIR="./schedtest_$NOW"
mkdir $OUTDIR || exit 1

ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $0`

tail -n+$ARCHIVE $0 | tar xj -C $OUTDIR 2>&1 | grep -v 'in the future'

if [ "x${1^^}" != "xRUN" ]; then
  echo
  echo "SchedTest Suite binary package successfully installed!"
  echo
  echo "To know about available execution options:"
  echo "  $ cd $OUTDIR && ./schedtest -h"
  echo
  echo
  exit 0
fi

# Notice the user that the SchedTest suite will be executed using
# the default embedded configuration.
# This mode is enabled by passing the "run" parameter to the
# SchedTest suite installation package.
echo
echo "Going to run SchedTest with default (embedded) configuration..."
echo "Press CTRL+C in 5[s] to abort!"
sleep 5

CDIR=`pwd`
cd $OUTDIR
./schedtest
RESULT=$?

if [ $RESULT -ne 0 ]; then
  rm -rf $OUTDIR
  rm -f schedtest_lastrun
fi

cd $CDIR

ln -s $OUTDIR schedtest_lastrun
exit 0

# Note: we need an empty line after this
__ARCHIVE_BELOW__
