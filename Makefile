.PHONY: rescript-clean rescript-build rescript-start
.PHONY: webpack webpack-prod webpack-dev-server
.PHONY: clean build start

NODE_BINS = node_modules/.bin

EXAMPLE_DIR = example

rescript-clean:
	$(NODE_BINS)/rescript clean -with-deps

rescript-build:
	$(NODE_BINS)/rescript

rescript-start:
	mkdir $(EXAMPLE_DIR)/build; \
	$(NODE_BINS)/rescript build -w

build-example:
	node --experimental-loader=./src/node-loader.mjs $(EXAMPLE_DIR)/src/ExampleBuild.bs.js

start-example:
	node --experimental-loader=./src/node-loader.mjs $(EXAMPLE_DIR)/src/ExampleStart.bs.js

serve-example:
	$(NODE_BINS)/serve -l 3005 $(EXAMPLE_DIR)/build/bundle

clean:
	rm -rf $(EXAMPLE_DIR)/build
	mkdir $(EXAMPLE_DIR)/build
	make rescript-clean

build: clean
	make rescript-build
	make build-example

start: clean rescript-build
	make -j 2 rescript-start start-example

init-dev:
	rm -rf _opam
	opam switch create . 4.06.1 --deps-only

fmt:
	@$(NODE_BINS)/bsrefmt --in-place -w 80 \
	$(shell find ./src ./example -type f \( -name *.re -o -name *.rei \))

webpack-bundle-analyzer:
	@$(NODE_BINS)/webpack-bundle-analyzer $(EXAMPLE_DIR)/build/bundle/stats.json
