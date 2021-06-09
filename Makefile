ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
UNAME := $(shell uname)

.PHONY:all
all: mproton_cgo_exports.h build-ui build self-test

mproton_cgo_exports.h:
	go tool cgo -exportheader mproton_cgo_exports.h mproton.go

.PHONY: setup-dev-env
setup-dev-env:
	$(ROOT_DIR)/scripts/$(UNAME)/install
	$(MAKE) -C ./cmd/exampleapp/protonappui setup-dev-env

.PHONY: install-build-deps
install-build-deps:
	$(MAKE) -C ./cmd/exampleapp/protonappui install-build-deps

.PHONY: build-ui
build-ui:
	$(MAKE) -C ./cmd/exampleapp/protonappui build

.PHONY: build 
build:
	cd ./cmd/exampleapp && go build -v

.PHONY: build
self-test: 
	$(ROOT_DIR)/cmd/exampleapp/exampleapp --self-test

