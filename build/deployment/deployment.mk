
################################################################################
# Installation package SETUP
################################################################################

# Import SchedTest configuration into environment
-include $(BASE_DIR)/build/configuration/schedtest-config.env

# Installation packages
export PKG=$(BASE_DIR)/build/deployment/schedtest.tar.bz2
export BSX=$(BASE_DIR)/$(PACKAGE_NAME).bsx

# Self installer script
export BSX_OUT=$(BASE_DIR)/out
export BSX_DIR=$(BASE_DIR)/build/deployment
export BSX_INS=$(BASE_DIR)/build/deployment/schedtest
export STS_RES=$(BASE_DIR)/build/configuration

# Setup for deployment to target device
export TRG_PATH=$(CONFIG_TARGET_DEPLOYMENT_PATH)
ifdef CONFIG_TARGET_ANDROID
export TRG="Android"
export TRG_DEV="$(CONFIG_ADB_USB_DEVICE)"
endif # CONFIG_TARGET_ANDROID
ifdef CONFIG_TARGET_LINUX
export TRG="Linux"
export TRG_IP="$(CONFIG_TARGET_IP_ADDRESS)"
export TRG_USER=$(CONFIG_TARGET_USERNAME)
endif # CONFIG_TARGET_LINUX

# Paths and tools
export STRIP=$(shell echo $(CXX) | sed '{s/g++\(-[0-9\.]\+\)\?$$/strip/}')

################################################################################
# Installation package BUILDING
################################################################################
.PHONY: package-build

deployment: package-build
package-build: build install
	@echo
	@echo "==== Building Binary Package"
	@echo "Target.................. "$(CONFIG_TARGET_NAME)": "$(CONFIG_TARGET_DESCRIPTION)
	@echo "Installation path....... "$(TRG_PATH)
	@rm -rf $(PKG) &>/dev/null
	@rm -rf $(BSX) &>/dev/null
	@echo
	@echo "Build new intallation package [$(BSX)]..."
	@$(BSX_DIR)/build.sh
	@echo

