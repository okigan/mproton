.PHONY: build
build:
	go build -v ./...

.PHONY: install
install:
	add-apt-repository -y ppa:longsleep/golang-backports
	apt update
	apt install -y golang-go libgtk-3-dev libwebkit2gtk-4.0-dev libappindicator3-dev
	

.PHONY: self-test
self-test:


