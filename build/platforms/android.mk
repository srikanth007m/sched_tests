
################################################################################
# Platform setup and checks
#

CONNSTATE = $(shell adb get-state)

ifneq ($(CONFIG_CALIBRATION_FILENAME),"")
CALIB =	$(BASE_DIR)/build/platforms/$(CONFIG_CALIBRATION_FILENAME)
endif # CONFIG_CALIBRATION_FILNAMEE

platform_check:
ifneq ($(CONNSTATE),device)
ifdef CONFIG_ADB_IP
	@echo "Connect Android device @ " CONFIG_AND_IP_ADDRESS ":" CONFIG_ADB_IP_PORT
	@adb connect $(CONFIG_ADB_IP_ADDRESS):$(CONFIG_ADB_IP_PORT)
endif
	@echo "Waiting for Android devices..."
	@adb wait-for-device
	@echo "Restaring ADB server as root..."
	@adb root
endif # CONNSTATE
	@adb devices

platform_wipe:
ifdef CONFIG_TARGET_WIPE
	@echo
	@echo "==== Wipe target installation folder [$(DEPLOYMENT_DIR)] ===="
ifneq ($(CONNSTATE),device)
	@echo "Device not present: wipe skipped"
else
	@adb shell "rm -rf $(DEPLOYMENT_DIR)/*"
endif
endif

platform_setup: platform_check platform_wipe
	@echo
	@echo "==== Setup target installation folder [$(DEPLOYMENT_DIR)] ===="
	adb shell 'mount -o remount,rw /'
	adb shell '[ -d $(DEPLOYMENT_DIR) ] || mkdir -p $(DEPLOYMENT_DIR)'
	adb shell 'if [ ! -f /bin/sh ]; then mkdir -p /bin/ && cp /system/bin/sh /bin/; fi'
	adb shell 'ls -la $(DEPLOYMENT_DIR)'

################################################################################
# Remote intallation
#

push_calibration:
ifdef CONFIG_BACKUP_CALIBRATION_FILE
	@if [ -f $(CALIB) ]; then \
	 echo ; \
	 echo "==== Push calibration file ===" ; \
	 echo "Using calibration file: $(CALIB)" ; \
	 adb push $(CALIB) $(DEPLOYMENT_DIR)/; \
	fi
endif # CONFIG_BACKUP_CALIBRATION_FILE

push_suites:
	@echo $(LOG)
	@echo "==== Push suites to [$(CONFIG_TARGET_NAME)] ===" $(LOG)
	@adb push $(INSTALL_DIR)/ $(DEPLOYMENT_DIR)


################################################################################
# Remote execution
#

run: platform_check $(RUNDIRS)
$(RUNDIRS):
	@echo $(LOG)
	@echo "==== Running test suite [$(@:run-%_suite=%)] ====" $(LOG)
	@echo "./testrunner --suite $(@:run-%_suite=%) --run $(TAG) --n $(CONFIG_ITERATIONS) --verbose --all" $(LOG)
	@adb shell 'cd $(DEPLOYMENT_DIR)/ && \
		./testrunner --suite $(@:run-%_suite=%) --run $(TAG) --n $(CONFIG_ITERATIONS) --verbose --all' $(LOG)

.PHONY: run $(RUNDIRS)


################################################################################
# Results collection
#

pull_calibration:
ifdef CONFIG_BACKUP_CALIBRATION_FILE
	@echo
	@echo "==== Backup calibration file [$(CALIB)] ==="
	@adb pull $(DEPLOYMENT_DIR)/$(CONFIG_CALIBRATION_FILENAME) $(INSTALL_DIR)/
	@cp $(INSTALL_DIR)/$(CONFIG_CALIBRATION_FILENAME) $(CALIB)
endif # CONFIG_BACKUP_CALIBRATION_FILE

pull_events:
	@echo $(LOG)
	@echo "==== Pull FTrace events ====" $(LOG)
	@echo "Collecting FTrace events..." $(LOG)
	@adb shell "cd $(DEPLOYMENT_DIR) && busybox tar cf \
		$(subst $(DEPLOYMENT_DIR)/,,$(DEPLOYMENT_EVENTS_DIR)).tar \
		$(subst $(DEPLOYMENT_DIR)/,,$(DEPLOYMENT_EVENTS_DIR))"
	@echo -n "Pulling FTrace events archive... "
	@adb pull $(DEPLOYMENT_EVENTS_DIR).tar $(INSTALL_DIR)

pull_ftraces:
	@echo $(LOG)
	@echo "==== Pull FTrace archives ====" $(LOG)
	@cd $(INSTALL_DIR) && \
		adb shell 'find $(DEPLOYMENT_SUITES_DIR) -name trace.ftrace.gz' | \
		tr '\r' ' ' | \
		sed 's|$(subst $\",,$(DEPLOYMENT_DIR))/||' | \
		while read FTRACE; do \
			echo -n "Pull $$FTRACE\t"; \
			adb pull $(subst $\",,$(DEPLOYMENT_DIR))/$$FTRACE $$FTRACE; \
		done

pull_results:
	@echo
	@echo "==== Pull test results ====" $(LOG)
	@cd $(INSTALL_DIR) && \
		adb shell 'find $(DEPLOYMENT_SUITES_DIR) -name "*.res"' | \
		tr '\r' ' ' | \
		sed 's|$(subst $\",,$(DEPLOYMENT_DIR))/||' | \
		while read RESULT; do \
			echo -n "Pull $$RESULT\t"; \
			adb pull $(subst $\",,$(DEPLOYMENT_DIR))/$$RESULT $$RESULT; \
		done
	@echo

