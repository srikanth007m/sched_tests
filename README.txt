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

This is the test framework and the accompanying test suites for the
big.LITTLE MP scheduler extensions from ARM.

Complete documentation describing the installation and usage of the
framework and the suites is provided in HTML format at:

  ./docs/index.html

1. Preface

    a.  Proprietary notice

        Copyright (c) 2015, ARM Limited

    b.  License details

	This software is release under the terms of the GLPv2 license.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.
        If not, see <http://www.gnu.org/licenses/>.

2.  Release details

    a.  About

        This release includes the big.LITTLE MP validation asset. The main
        component of big.LITTLE MP validation asset included here is:

        SchedTestSuite - validating functional aspects of big.LITTLE MP
        implementation.

        The current release is v3.0.

        Note: Please refer to the relevant HTML documenation for
        further details about the components included in this
        release.

    b.  Change History

	Version 3.0:
                  - Overall refactoring to support better
                    configurability via Kconfig menus.
                  - Integrated support for Android and Linux
                    targets.
                  - Added SchedTest shell to easy users interaction
                    with the suite.
                  - Added IRQ checking test suite.
                  - Added Thresholds validation test suite.
                  - Added Use-Cases based test suite.
                  - Added IPA test suite.

        Version 2.1:
                  - Added new test suite for validating small task
                    packing functionality.
                  - Extended basiccheck test in basic_suite to adapt
                    with change in topology.
                  - Fixed loadbalance_suite tests to discover big
                    and LITTLE CPUs at runtime.

        Version 2.0:
                  - Major restructuring of code separating suites
                    and framework components into different
                    locations and general code cleanup.
                  - Fix for stability_test_scn06.1 hang.

        Version 1.1:
                  - Initial release version of the test suite

3.  Support

    ARM welcomes any feedback on this release. Please send feedback to
    support@arm.com.
