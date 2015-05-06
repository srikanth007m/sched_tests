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

# Load current SchedTest suite configuration
. build/configuration/schedtest-config

# Move into installation folder
pushd $BSX_OUT &>/dev/null

# Setup paths and user in the installation file
cat ${BSX_INS}.in | sed -e "\
	s;__TRG__;${TRG//\"/};;
	s;__DEVICE__;${TRG_DEV//\"/};;
	s;__IP__;${TRG_IP//\"/};;
	s;__USER__;${TRG_USER//\"/};;
	s;__PATH__;${TRG_PATH//\"/};;" > ./schedtest
chmod a+x ./schedtest
cp $STS_RES/schedtest_colors $STS_RES/schedtest_results ./common/

# Archive content of installation folder
tar cjf $PKG .
popd &>/dev/null

if [ -e $PKG ]; then
	cat $BSX_DIR/decompress.sh $PKG > $BSX
	chmod +x $BSX
else
    echo "Self-installing update package creation: FAILED!"
    echo "   update package [$PKG] does not exist"
    exit 1
fi

echo "Installation package created:"
echo "  $BSX"
exit 0
