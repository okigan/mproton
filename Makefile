ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
UNAME := $(shell uname)

.PHONY: build
build:
	go build -v ./...

.PHONY: install
install:
	$(ROOT_DIR)/scripts/$(UNAME)/install
	

.PHONY: self-test
self-test:
