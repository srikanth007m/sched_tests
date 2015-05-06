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

# Configuration setup
setup: setup_config setup_build setup_check

.PHONY: setup_config setup_build setup_check


################################################################################
# Build Configuration Setup (using KConfig)
################################################################################

include build/core/config.mk
clean_config:
	@echo
	@echo "==== Clean-up configuration ===="
	@rm build/configuration/schedtest-config* 2>/dev/null || echo -n

clean_logs:
	@find . -name "buildlog*" | xargs rm -rf

clean_install:
	@echo
	@echo "==== Cleaning local installation [$(INSTALL_DIR)] ===="
	@rm -rf $(INSTALL_DIR)/*

clean: clean_install clean_logs
	@echo

distclean: clean clean_suites platform_distclean clean_config
	@echo

.PHONY: clean_config clean_logs clean_install distclean


################################################################################
# Test Suites
################################################################################

.PHONY: suites clean_suites
suites:
clean_suites:

-include build/core/suites-platforms.mk
-include build/core/suites-sched.mk
-include build/core/suites-stability.mk
-include build/core/suites-ipa.mk


################################################################################
# Platform Specific Definitions
################################################################################

include build/core/platform.mk
include build/core/deployment.mk


################################################################################
# Components
################################################################################

.PHONY: components clean_components
components:
clean_components:

-include build/core/components.mk


################################################################################
# Documentation
################################################################################

.PHONY: docs clean_docs
docs:
clean_docs:

include docs/documentation.mk



################################################################################
# Main
################################################################################

include build/core/common.mk

