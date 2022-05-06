.PHONY: rescript-clean rescript-build rescript-start
.PHONY: webpack webpack-prod webpack-dev-server
.PHONY: clean build start

NODE_BINS = node_modules/.bin

EXAMPLE_DIR = example

rescript-clean:
	$(NODE_BINS)/rescript clean -with-deps

rescript-build: rescript-clean
	$(NODE_BINS)/rescript

rescript-start:
	mkdir $(EXAMPLE_DIR)/build; \
	$(NODE_BINS)/rescript build -w

webpack-dev:
	NODE_ENV=development $(NODE_BINS)/webpack

webpack-prod:
	NODE_ENV=production $(NODE_BINS)/webpack

webpack-dev-server:
	$(NODE_BINS)/webpack-dev-server --open

clean:
	make rescript-clean

build: clean
	make rescript-build
	make build-page-template
	make webpack-prod

start: clean
	make rescript-build; build-page-template; make -j 2 rescript-start webpack-dev-server

build-example:
	rm -rf $(EXAMPLE_DIR)/build
	mkdir $(EXAMPLE_DIR)/build
	node $(EXAMPLE_DIR)/src/ExampleBuild.bs.js

serve-example:
	npx serve -l 3005 $(EXAMPLE_DIR)/build

init-dev:
	rm -rf _opam
	opam switch create . 4.06.1 --deps-only
