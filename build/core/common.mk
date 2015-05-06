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

targets:
	@echo "Main build targets:"
	@echo "- clean		clean all tests and utilities"
	@echo "- build		build all tests and utilities"
	@echo "- install	copy all external utilities into test suites"
	@echo "- push		send all test suites to target - connect device with adb first"
	@echo "- run		run all test suites on host - connect device with adb first"
	@echo "- pull		fetch all test suite directorys - connect device with adb first"
	@echo "- archive	make tar.gz of all suites and logfiles"
	@echo "- all		build, install, push, run, pull, archive configured suites"

all: setup build install push run pull archive
.PHONY: setup build install push run pull archive


################################################################################
# Build configured components

build-all: $(BUILDALL)
$(BUILDALL): setup_check
	@echo $(LOG)
	@echo "==== Building [$(@:build-%=%)] ====" $(LOG)
	$(MAKE) -C $(@:build-%=%) CROSS_COMPILE=$(CONFIG_CROSS_COMPILER) all $(LOG)

build-android: $(BUILDANDROID)
$(BUILDANDROID): setup_check
	@echo $(LOG)
	@echo "==== Building Android component [$(@:android-%=%)] ====" $(LOG)
	$(MAKE) -C $(@:android-%=%) CROSS_COMPILE=$(CONFIG_CROSS_COMPILER) android $(LOG)

build: setup build-all build-android

.PHONY: build-all build-android build
.PHONY:	$(BUILDALL) $(BUILDANDROID)


################################################################################
# Local installation

INSTALL_DIRS = TOOLS_DIR=$(INSTALL_TOOLS_DIR) SUITES_DIR=$(INSTALL_SUITES_DIR)

$(INSTALLALL): installall-% : build-%
	$(MAKE) -C $(@:installall-%=%) $(INSTALL_DIRS) install $(LOG)

$(INSTALLANDROID): installandroid-% : android-%
	$(MAKE) -C $(@:installandroid-%=%) $(INSTALL_DIRS) install $(LOG)

$(INSTALLBIN):
	$(MAKE) -C $(@:installbin-%=%) $(INSTALL_DIRS) install $(LOG)

# autogenerate build dependencies where they exist
define BUILDALL_TARGET
install-$(1): installall-$(1)
endef
$(foreach target,$(TMPINSTALLALL),$(eval $(call BUILDALL_TARGET,$(target))))

define BUILDANDROID_TARGET
install-$(1): installandroid-$(1)
endef
$(foreach target,$(TMPINSTALLANDROID),$(eval $(call BUILDANDROID_TARGET,$(target))))

define BUILDBIN_TARGET
install-$(1): installbin-$(1)
endef
$(foreach target,$(TMPINSTALLBIN),$(eval $(call BUILDBIN_TARGET,$(target))))

install_registry: setup $(INSTALLALL) $(INSTALLANDROID) $(INSTALLBIN)
	@echo $(LOG)
	@echo "==== Update suites registry ====" $(LOG)
	@echo "generate suites configuration file: $(CONF_SUITES_TXT)" $(LOG)
	[ ! -f $(CONF_SUITES_TXT) ] || rm -f $(CONF_SUITES_TXT)
	ls out/suites | while read SUITE; do \
		echo $${SUITE} $(DEPLOYMENT_SUITES_DIR)/$${SUITE} >> $(CONF_SUITES_TXT); \
	 done

install_testrunner: install_registry $(INSTALL_DIR)/testrunner
$(INSTALL_DIR)/testrunner:
	@echo $(LOG)
	@echo "==== Installing testrunner and run script ====" $(LOG)
	@cp $(TESTRUNNER_BINARY) $(INSTALL_DIR)/testrunner $(LOG)
	@cp $(TESTRUNNER_SCRIPT) $(INSTALL_DIR)/schedtest-run $(LOG)

install_configuration:
	@echo $(LOG)
	@echo "==== Installing configuration ====" $(LOG)
	@cp build/configuration/schedtest-config $(INSTALL_DIR)
	@sed -i 's|CONFIG_|export CONFIG_|' $(INSTALL_DIR)/schedtest-config

install_initenv: install_configuration $(CONF_INIT_ENV)
$(CONF_INIT_ENV):
	@echo $(LOG)
	@echo "==== Update environment setup ====" $(LOG)
	@echo "environment setup file: $(CONF_INIT_ENV)" $(LOG)
	@echo "# Source this file to setup the test suite environment" > $(CONF_INIT_ENV)
	@echo "export BASEDIR='$(DEPLOYMENT_DIR)'" >> $(CONF_INIT_ENV)
	@echo "export TOOLS='$(DEPLOYMENT_TOOLS_DIR)'" >> $(CONF_INIT_ENV)
	@echo "export SUITES='$(DEPLOYMENT_SUITES_DIR)'" >> $(CONF_INIT_ENV)
	@echo "export CALIBRATION='$(CONF_CALIB_TXT)'" >> $(CONF_INIT_ENV)
	@echo "export FTRACE_EVENTS='$(DEPLOYMENT_EVENTS_DIR)'" >> $(CONF_INIT_ENV)
	@echo "" >> $(CONF_INIT_ENV)
	@echo "# Add test suite configuration" >> $(CONF_INIT_ENV)
	@echo "source $(CONFIG_TARGET_DEPLOYMENT_PATH)/schedtest-config" >> $(CONF_INIT_ENV)
	@echo $(LOG)

install_extras: install_testrunner install_initenv

install: build install_suites install_extras
	@echo $(LOG)
	@echo "==== SchedTest local installation COMPLETED ====" $(LOG)
	@echo "Version: $(git describe --tags)" $(LOG)
	@echo "Target: $(_schedtest-get CONFIG_TARGET_PLATFORM)" $(LOG)
	@echo "Board: $(_schedtest-get CONFIG_TARGET_DESCRIPTION)" $(LOG)
	@echo "Suite installed on: " $(LOG)
	@echo "   $(INSTALL_DIR)" $(LOG)
	@echo $(LOG)
	@ls -la $(INSTALL_DIR) $(LOG)
	@echo "================================================" $(LOG)
	@echo $(LOG)

.PHONY: install_registry install_testrunner install_configuration
.PHONY: install_initenv install_extras install
.PHONY: $(INSTALLDIRS) $(INSTALLALL) $(INSTALLANDROID) $(INSTALLBIN)


################################################################################
# Target push

push: platform_setup push_suites push_calibration
	@echo $(LOG)
	@echo "==== SchedTest target installation COMPLETED ====" $(LOG)
	@echo "Suite installed on: " $(LOG)
	@echo "   $(DEPLOYMENT_DIR)" $(LOG)
	@echo "================================================" $(LOG)
	@echo $(LOG)

.PHOMY: push push_suites push_calibration


################################################################################
# Target pull

pull: pull_results pull_events pull_ftraces pull_calibration
	@echo $(LOG)
	@echo "==== SchedTest results pull COMPLETED ====" $(LOG)
	@echo "Results copied into: " $(LOG)
	@echo "   $(DEPLOYMENT_DIR)" $(LOG)
	@echo "================================================" $(LOG)
	@echo $(LOG)

.PHOMY: pull_results pull_events pull_ftraces pull_calibration


################################################################################
# Local cleanups

clean: $(CLEANDIRS)
$(CLEANDIRS):
	@echo $(LOG)
	@echo "==== Cleaning [$(@:clean-%_suite=%)] ====" $(LOG)
	$(MAKE) -C $(@:clean-%=%) $(INSTALL_DIRS) clean $(LOG)

.PHONY: clean $(CLEANDIRS)


################################################################################
# Test suite archiving

archive:
	@echo $(LOG)
	@echo "==== Archiving test suite ====" $(LOG)
	@echo "Archiving into testrun_$(TAG).tar.gz" $(LOG)
	@cp $(LOGFILE) $(INSTALL_DIR)
	@tar cfhz testrun_$(TAG).tar.gz -C $(INSTALL_DIR) .

.PHONY: archive

