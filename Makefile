.PHONY: build
build:
	go build -v ./...

.PHONY: install
install:
	./scripts/linux/install
	

.PHONY: self-test
self-test:


