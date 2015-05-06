
.PHONY: release_build release_apply

release_build: build
release_apply: release_build

include build/deployment/deployment.mk
