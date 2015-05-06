
################################################################################
# Build System Setup
################################################################################

# Building system configuration
BASE_DIR := $(shell pwd)
BUILD_SYSTEM := $(TOPDIR)build/core

# Host installation path
INSTALL_DIR := $(BASE_DIR)/out
INSTALL_TOOLS_DIR = $(INSTALL_DIR)/$(CONFIG_TOOLS_DIRNAME)
INSTALL_SUITES_DIR = $(INSTALL_DIR)/$(CONFIG_SUITES_DIRNAME)

# Targe installation paths
DEPLOYMENT_DIR = $(CONFIG_TARGET_DEPLOYMENT_PATH)
DEPLOYMENT_TOOLS_DIR = $(DEPLOYMENT_DIR)/$(CONFIG_TOOLS_DIRNAME)
DEPLOYMENT_SUITES_DIR = $(DEPLOYMENT_DIR)/$(CONFIG_SUITES_DIRNAME)
DEPLOYMENT_EVENTS_DIR = $(DEPLOYMENT_DIR)/$(CONFIG_EVENTS_DIRNAME)

# Setup default target (required for cold-and-dark startup)
CONFIG_TARGET_PLATFORM := android

# KConfig configuration
KCONFIG_PATH := $(BASE_DIR)/components/kconfig/
KCONFIG_CONFIG := $(BASE_DIR)/build/configuration/schedtest-config
KCONFIG_FILENAME := $(BASE_DIR)/build/configuration/Kconfig

KCONFIG_TMP := $(BASE_DIR)/build/configuration/schedtest-config.tmp
KCONFIG_MOD := $(BASE_DIR)/build/configuration
KCONFIG_CMK := $(BASE_DIR)/build/core

include $(KCONFIG_PATH)/kconfig.inc.mk

.PHONY: boostrap
bootstrap:
	$(info )
	$(info ==== Bootstraping Building System ====)
	@find $(BASE_DIR) -name 'Kconfig' -o -name '*.mki' \
		| awk '/Kconfig/{print "source \"" $$0 "\""} /.mki/{print "-include " $$0}' \
		| sed -e 's|$(BASE_DIR)/||' \
		| sort \
		> $(KCONFIG_TMP)
	@echo 'Looking up for test components...'
	@echo
	@echo '.:: Components'
	@grep 'components/.*/Kconfig' $(KCONFIG_TMP) \
		> $(KCONFIG_MOD)/Kconfig-components
	@sed -e 's/\/Kconfig//' -e 's/source //' -e 's/"//g' \
		$(KCONFIG_MOD)/Kconfig-components
	@grep 'components/.*/*.mki' $(KCONFIG_TMP) \
		> $(KCONFIG_CMK)/components.mk
	@echo
	@echo 'Looking up for test suites...'
	@echo
	@echo '.:: Platform Suites'
	@grep 'suites/platform/.*/Kconfig' $(KCONFIG_TMP) \
		> $(KCONFIG_MOD)/Kconfig-suites-platforms
	@sed -e 's/\/Kconfig//' -e 's/source //' -e 's/"//g' \
		$(KCONFIG_MOD)/Kconfig-suites-platforms
	@grep 'suites/platform/.*/*.mki' $(KCONFIG_TMP) \
		> $(KCONFIG_CMK)/suites-platforms.mk
	@echo
	@echo '.:: Scheduler Suites'
	@grep 'suites/sched/.*checks/.*/Kconfig' $(KCONFIG_TMP) \
		> $(KCONFIG_MOD)/Kconfig-suites-sched-checks
	@sed -e 's/\/Kconfig//' -e 's/source //' -e 's/"//g' \
		$(KCONFIG_MOD)/Kconfig-suites-sched-checks
	@grep 'suites/sched/.*basic_features/.*/Kconfig' $(KCONFIG_TMP) \
		> $(KCONFIG_MOD)/Kconfig-suites-sched-basic-features
	@sed -e 's/\/Kconfig//' -e 's/source //' -e 's/"//g' \
		$(KCONFIG_MOD)/Kconfig-suites-sched-basic-features
	@grep 'suites/sched/.*advanced_features/.*/Kconfig' $(KCONFIG_TMP) \
		> $(KCONFIG_MOD)/Kconfig-suites-sched-advanced-features
	@sed -e 's/\/Kconfig//' -e 's/source //' -e 's/"//g' \
		$(KCONFIG_MOD)/Kconfig-suites-sched-advanced-features
	@grep 'suites/sched/.*/*.mki' $(KCONFIG_TMP) \
		> $(KCONFIG_CMK)/suites-sched.mk
	@echo
	@echo '.:: Stability Suites'
	@grep 'suites/stability/.*/Kconfig' $(KCONFIG_TMP) \
		> $(KCONFIG_MOD)/Kconfig-suites-stability
	@sed -e 's/\/Kconfig//' -e 's/source //' -e 's/"//g' \
		$(KCONFIG_MOD)/Kconfig-suites-stability
	@grep 'suites/stability/.*/*.mki' $(KCONFIG_TMP) \
		> $(KCONFIG_CMK)/suites-stability.mk
	@echo
	@rm $(KCONFIG_TMP)

setup_config:
	$(info )
	$(info ==== Checking build configuration ====)
ifndef CONFIG_DONE
	$(info )
	$(info ERROR: test suite not yet configured!)
	$(info INFO:  start a new configuration with: make menuconfig)
	$(info )
	$(error Build aborted)
endif
ifdef CONFIG_TARGET_LINUX
	$(info Configured for a Linux platform)
endif
ifdef CONFIG_TARGET_ANDROID
	$(info Configured for a Android platform)
endif
	$(info Target: $(CONFIG_TARGET_DESCRIPTION))

setup_build: $(INSTALL_DIR)/common
$(INSTALL_DIR)/common:
	@[ -d $(INSTALL_DIR) ]        || mkdir $(INSTALL_DIR)
	@[ -d $(INSTALL_DIR)/$(CONFIG_SUITES_DIRNAME) ] || mkdir $(INSTALL_DIR)/$(CONFIG_SUITES_DIRNAME)
	@[ -d $(INSTALL_DIR)/$(CONFIG_TOOLS_DIRNAME) ] || mkdir $(INSTALL_DIR)/$(CONFIG_TOOLS_DIRNAME)

setup_check:
	@echo
	@echo "==== Checking cross-toolchain ===="
	@$(shell export CROSS_COMPILE=$(CONFIG_CROSS_COMPILER))
	@echo "Checking for [$(CONFIG_CROSS_COMPILER)] cross-compiler availability:"
	@which $(CONFIG_CROSS_COMPILER)gcc

################################################################################
# Build Targets Initialization
################################################################################

TESTRUNNER_BINARY :=
TESTRUNNER_SCRIPT :=
MAKE_ALL :=
MAKE_INSTALL :=
MAKE_ANDROID :=
MAKE_CLEAN :=
SUITES :=
SUITES_DIR :=

# temporary build names for suites to allow push, pull and run targets
RUNDIRS  = $(SUITES:%=run-%)

# temporary build name for cleaning
CLEANDIRS = $(MAKE_CLEAN:%=clean-%)

# temporary build name for compilation targets
BUILDALL = $(MAKE_ALL:%=build-%)
BUILDANDROID = $(MAKE_ANDROID:%=android-%)
INSTALLDIRS = $(MAKE_INSTALL:%=install-%)
TMPINSTALLBIN = $(filter-out $(MAKE_ALL) $(MAKE_ANDROID), $(MAKE_INSTALL))
INSTALLBIN = $(TMPINSTALLBIN:%=installbin-%)
TMPINSTALLANDROID = $(filter $(MAKE_INSTALL),$(MAKE_ANDROID))
INSTALLANDROID = $(TMPINSTALLANDROID:%=installandroid-%)
TMPINSTALLALL = $(filter $(MAKE_INSTALL),$(MAKE_ALL))
INSTALLALL = $(TMPINSTALLALL:%=installall-%)

# timestamp for tagging test runs
TAG := $(shell date +"%d-%m-%Y-%H-%M-%S")
LOGFILE := buildlog_$(TAG).txt
LOG := | tee -a $(LOGFILE)

################################################################################
# Test suite configuration files
CONF_SUITES_TXT = $(INSTALL_DIR)/$(CONFIG_SUITES_FILENAME)
CONF_INIT_ENV   = $(INSTALL_DIR)/$(CONFIG_INIT_ENV_FILENAME)
CONF_CALIB_TXT  = $(DEPLOYMENT_DIR)/$(CONFIG_CALIBRATION_FILENAME)

