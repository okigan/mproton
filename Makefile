UNAME := $(shell uname)

.PHONY: build
build:
	go build -v ./...

.PHONY: install
install:
	./scripts/$(UNAME)/install
	

.PHONY: self-test
self-test:


