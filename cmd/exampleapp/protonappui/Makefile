.PHONY:all

.PHONY:setup-build-env
setup-build-env:
	npm install yarn

install-build-deps:
	PATH=./node_modules/.bin/:$(PATH) yarn install

build:
	PATH=./node_modules/.bin/:$(PATH) yarn build
