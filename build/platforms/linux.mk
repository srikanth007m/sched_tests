
################################################################################
# Platform setup and checks
#

PINGOK = $(shell ping -c1 $(CONFIG_TARGET_IP_ADDRESS) >/dev/null && echo OK || echo KO)

ifneq ($(CONFIG_TARGET_SSH_KEY),"")
SSHKEY = $(BASE_DIR)/build/deployment/linux/$(CONFIG_TARGET_SSH_KEY)
endif # CONFIG_TARGET_SSH_KEY

ifneq ($(CONFIG_CALIBRATION_FILENAME),"")
CALIB =	$(BASE_DIR)/build/platforms/$(CONFIG_CALIBRATION_FILENAME)
endif # CONFIG_CALIBRATION_FILENAME

SSHCONF = $(BASE_DIR)/build/deployment/ssh_config
SSH = ssh -F $(SSHCONF) target
SCP = scp -F $(SSHCONF)

platform_check:

platform_wipe:
ifdef CONFIG_TARGET_WIPE
	@echo
	@echo "==== Wipe target installation folder [$(DEPLOYMENT_DIR)] ===="
ifneq ($(PINGOK),OK)
	@echo "Device not present: wipe skipped"
else
	@$(SSH) 'rm -rf $(DEPLOYMENT_DIR)/*'
endif
endif

platform_conf:
	@echo "Update SSH configuration file..."
	@echo "Host target" > $(SSHCONF)
	@echo "  HostName $(CONFIG_TARGET_IP_ADDRESS)" >> $(SSHCONF)
	@echo "  User $(CONFIG_TARGET_USERNAME)" >> $(SSHCONF)
ifneq ($(CONFIG_TARGET_SSH_KEY),"")
	@echo "  IdentityFile $(SSHKEY)" >> $(SSHCONF)
endif # CONFIG_TARGET_SSH_KEY
	@cat $(SSHCONF)
	@echo
	@echo "Setup target installation folder ($(DEPLOYMENT_DIR))..."
	@$(SSH) '[ -d $(DEPLOYMENT_DIR) ] || mkdir -p $(DEPLOYMENT_DIR)'
	@$(SSH) 'if [ ! -f /bin/bash ]; then ln -s /bin/sh /bin/bash; fi'
	@$(SSH) 'ls -la $(DEPLOYMENT_DIR)'

platform_setup: platform_conf platform_check platform_wipe


################################################################################
# Remote intallation
#

push_calibration:
ifneq ($(CONFIG_CALIBRATION_FILENAME),"")
	@echo
	@if [ -f $(CALIB) ]; then \
	 echo ; \
	 echo "==== Push calibration file ===" ; \
	 echo "Using calibration file: $(CALIB)" ; \
	 $(SCP) $(CALIB) target:$(DEPLOYMENT_DIR)/; \
	fi
endif # CONFIG_BACKUP_CALIBRATION_FILE

push_suites:
	@echo $(LOG)
	@echo "==== Push suite to [$(CONFIG_TARGET_NAME)] ===" $(LOG)
	cd $(INSTALL_DIR) && $(SCP) -r * target:$(DEPLOYMENT_DIR)



################################################################################
# Remote execution
#

run: platform_check $(RUNDIRS)
$(RUNDIRS):
	@echo $(LOG)
	@echo "==== Running test suite [$(@:run-%_suite=%)] ====" $(LOG)
	@echo "./testrunner --suite $(@:run-%_suite=%) --run $(TAG) --n $(CONFIG_ITERATIONS) --verbose --all" $(LOG)
	@$(SSH) 'cd $(DEPLOYMENT_DIR)/ && \
		sudo ./testrunner --suite $(@:run-%_suite=%) --run $(TAG) --n $(CONFIG_ITERATIONS) --verbose --all' $(LOG)

.PHONY: run $(RUNDIRS)


################################################################################
# Results collection
#

pull_calibration:
ifdef CONFIG_BACKUP_CALIBRATION_FILE
	@echo $(LOG)
	@echo "Backup calibration file [$(CALIB)]" $(LOG)
	@$(SCP) target:$(DEPLOYMENT_DIR)/$(CONFIG_CALIBRATION_FILENAME) $(INSTALL_DIR)/
	@cp $(INSTALL_DIR)/$(CONFIG_CALIBRATION_FILENAME) $(CALIB)
endif # CONFIG_BACKUP_CALIBRATION_FILE

pull_events:
	@echo $(LOG)
	@echo "==== Pull FTrace events ====" $(LOG)
	@echo "Collecting FTrace events..." $(LOG)
	@$(SSH) "cd $(DEPLOYMENT_DIR) && tar cf \
		$(subst $(DEPLOYMENT_DIR)/,,$(DEPLOYMENT_EVENTS_DIR)).tar \
		$(subst $(DEPLOYMENT_DIR)/,,$(DEPLOYMENT_EVENTS_DIR))"
	@echo -n "Pulling FTrace events archive... "
	@$(SCP) target:$(DEPLOYMENT_EVENTS_DIR).tar $(INSTALL_DIR)

pull_ftraces:
	@echo $(LOG)
	@echo "==== Pull FTrace archives ====" $(LOG)
	@cd $(INSTALL_DIR) && \
		$(SSH) 'find $(DEPLOYMENT_SUITES_DIR) -name trace.ftrace.gz' | \
		tr '\r' ' ' | \
		sed 's|$(subst $\",,$(DEPLOYMENT_DIR))/||' | \
		while read FTRACE; do \
			echo -n "Pull $$FTRACE\t"; \
			$(SCP) target:$(subst $\",,$(DEPLOYMENT_DIR))/$$FTRACE $$FTRACE; \
		done

pull_results:
	@echo
	@echo "==== Pull test results ====" $(LOG)
	@cd $(INSTALL_DIR) && \
		$(SSH) 'find $(DEPLOYMENT_SUITES_DIR) -name "*.res"' | \
		tr '\r' ' ' | \
		sed 's|$(subst $\",,$(DEPLOYMENT_DIR))/||' | \
		while read RESULT; do \
			echo -n "Pull $$RESULT\t"; \
			$(SCP) target:$(subst $\",,$(DEPLOYMENT_DIR))/$$RESULT $$RESULT; \
		done
	@echo

