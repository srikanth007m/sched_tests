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

# Import test suite definitions
source ../../../../init_env

# Import functions library
source $TOOLS/functestlib.sh

DEFAULT_AFFINITY_FILE=/proc/irq/default_smp_affinity

if [ ! -f $DEFAULT_AFFINITY_FILE ]; then
  echo "Missing default affinity file [$DEFAULT_AFFINITY_FILE]"
  echo "Test aborted"
  exit 2
fi

getbigcpumask
BIGS_MASK="0x$__RET"
BIGS_MASK_DEC=`printf "%d" $BIGS_MASK`
echo "big CPUs mask: $BIGS_MASK"

DEFAULT_AFFINITY="0x`cat /proc/irq/default_smp_affinity`"
DEFAULT_AFFINITY_DEC=`printf "%d" $DEFAULT_AFFINITY`
echo "default affinity mask: $DEFAULT_AFFINITY"

IRQS_ON_BIGS=$((BIGS_MASK_DEC & DEFAULT_AFFINITY_DEC))
if [ $IRQS_ON_BIGS -ne 0 ]; then
  echo "ERROR: default IRQ mask [$DEFAULT_AFFINITY] includes big CPUs"
  echo "Test failed"
  exit 1
fi

echo "Test Passed"
exit 0

