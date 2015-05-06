

# Target platform
PLATFORM := $(shell echo $(CONFIG_TARGET_PLATFORM) | tr -d '"')

# Add platform-specific targets
include build/platforms/$(PLATFORM).mk

# Targets defined by platform-specific fragment
.PHONY: platform_setup platform_check platform_wipe platform_distclean

