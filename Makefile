.PHONY: rescript-clean rescript-build rescript-start
.PHONY: webpack webpack-prod webpack-dev-server
.PHONY: clean build start

NODE_BINS = node_modules/.bin

rescript-clean:
	$(NODE_BINS)/rescript clean -with-deps

rescript-build: rescript-clean
	$(NODE_BINS)/rescript

rescript-start:
	$(NODE_BINS)/rescript build -w

webpack:
	$(NODE_BINS)/webpack

webpack-prod:
	NODE_ENV=production $(NODE_BINS)/webpack

webpack-dev-server:
	$(NODE_BINS)/webpack-dev-server --open --hot

clean:
	make rescript-clean

build-page:
	node ./src/builder.mjs

build: clean
	make rescript-build

start: clean
	make rescript-build; make -j 2 rescript-start webpack-dev-server
