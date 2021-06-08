ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
UNAME := $(shell uname)

mproton_cgo_exports.h:
	go tool cgo -exportheader mproton_cgo_exports.h mproton.go

.PHONY: install
install:
	$(ROOT_DIR)/scripts/$(UNAME)/install
	

.PHONY: install-ui
install-ui:
	cd cmd/exampleapp/protonappui && yarn install

.PHONY: build-ui
build-ui:
	cd cmd/exampleapp/protonappui && yarn build

.PHONY: build 
build:
	go build -v ./...


.PHONY: build-exampleapp
build-exampleapp:
	cd $(ROOT_DIR)/cmd/exampleapp && go build -v ./...

.PHONY: self-test
self-test: build-exampleapp
	$(ROOT_DIR)/cmd/exampleapp/exampleapp --self-test

