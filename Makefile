MAKEFILE_DIR = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

NODE_BINS = node_modules/.bin

EXAMPLE_DIR = example

RESCRIPT_SSG_BIN = ENV_VAR=FOO ./src/js/bin.mjs

COMMANDS_DIR = $(EXAMPLE_DIR)/src/commands

.PHONY: clean-rescript
clean-rescript:
	$(NODE_BINS)/bsb -clean-world

.PHONY: build-rescript
build-rescript:
	$(NODE_BINS)/bsb -make-world

.PHONY: start-rescript
start-rescript:
	mkdir $(EXAMPLE_DIR)/build; \
	$(NODE_BINS)/bsb -make-world -w

.PHONY: build-example
build-example:
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) RESCRIPT_SSG_BUNDLER=esbuild $(RESCRIPT_SSG_BIN) $(COMMANDS_DIR)/Build.bs.js

.PHONY: start-example
start-example:
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) RESCRIPT_SSG_BUNDLER=esbuild $(RESCRIPT_SSG_BIN) $(COMMANDS_DIR)/Start.bs.js

.PHONY: serve-example
serve-example:
	$(NODE_BINS)/serve -l 3005 $(EXAMPLE_DIR)/build/public

.PHONY: clean-example
clean-example:
	rm -rf $(EXAMPLE_DIR)/build
	mkdir $(EXAMPLE_DIR)/build

.PHONY: clean
clean:
	make clean-test
	make clean-rescript
	make clean-example

.PHONY: build-webpack
build-webpack: clean
	make build-rescript
	make build-example

.PHONY: build-esbuild build
build-esbuild build: clean
	make build-rescript
	RESCRIPT_SSG_BUNDLER=esbuild make build-example

.PHONY: build-ci
build-ci: clean
	make build-rescript
	make test
	make clean-test
	make build-esbuild
	make clean-example
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) $(RESCRIPT_SSG_BIN) $(COMMANDS_DIR)/BuildWithTerser.bs.js
	make clean-example
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) $(RESCRIPT_SSG_BIN) $(COMMANDS_DIR)/BuildWithEsbuildPlugin.bs.js
	make clean-example
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) $(RESCRIPT_SSG_BIN) $(COMMANDS_DIR)/BuildWithTerserPluginWithEsbuild.bs.js
	make clean-example
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) $(RESCRIPT_SSG_BIN) $(COMMANDS_DIR)/BuildWithTerserPluginWithSwc.bs.js

.PHONY: build-serve
build-serve:
	make build-esbuild
	make serve-example

.PHONY: build-serve-webpack
build-serve-webpack:
	make build-webpack
	make serve-example

.PHONY: start
start: clean build-rescript
	make -j 2 start-rescript start-example

.PHONY: init-dev
init-dev:
	rm -rf _opam
	opam switch create . 4.06.1 --deps-only

.PHONY: format-reason
format-reason:
	@$(NODE_BINS)/bsrefmt --in-place -w 80 \
	$(shell find ./src ./example -type f \( -name *.re -o -name *.rei \))

.PHONY: format-rescript
format-rescript:
	@$(NODE_BINS)/rescript format -all

.PHONY: format
format:
	make format-reason
	make format-rescript

.PHONY: clean-test
clean-test:
	rm -rf tests/output
	rm -rf coverage

.PHONY: test
test: clean-test
	$(NODE_BINS)/c8 node ./tests/Tests.bs.js
