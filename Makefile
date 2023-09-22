MAKEFILE_DIR = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

project_name = rescript-ssg

DUNE = opam exec -- dune

.DEFAULT_GOAL := help

.PHONY: help
help: ## Print this help message
	@echo "List of available make commands";
	@echo "";
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}';
	@echo "";

.PHONY: create-switch
create-switch: ## Create opam switch
	opam switch create . 5.1.0 -y --deps-only

.PHONY: init
init: create-switch install ## Configure everything to develop this repository in local

.PHONY: install
install: ## Install development dependencies
	npm install
	opam update
	opam install -y . --deps-only --with-test

.PHONY: build
build: ## Build the project
	$(DUNE) build

.PHONY: build_verbose
build_verbose: ## Build the project in verbose mode
	$(DUNE) build --verbose

.PHONY: clean
clean: ## Clean build artifacts and other generated files
	$(DUNE) clean

.PHONY: format
format: ## Format the codebase with ocamlformat
	$(DUNE) build @fmt --auto-promote

.PHONY: format-check
format-check: ## Checks if format is correct
	$(DUNE) build @fmt

.PHONY: watch
watch: ## Watch for the filesystem and rebuild on every change
	$(DUNE) build --watch

.PHONY: test
test: ## Run the tests
	$(DUNE) build @runtest --no-buffer

.PHONY: test-watch
test-watch: ## Run the tests and watch for changes
	$(DUNE) build -w @runtest

MELANGE_ARTIFACTS_DIR = _build/default/app

RESCRIPT_SSG_BIN = ENV_VAR=FOO $(DUNE) exec rescript-ssg

NODE_BINS = node_modules/.bin

.PHONY: clean-example
clean-example: ## Clean example site artifacts
	rm -rf example/build

.PHONY: build-example
build-example: clean-example build ## Build the whole project and build example site
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) RESCRIPT_SSG_BUNDLER=esbuild $(RESCRIPT_SSG_BIN) $(MELANGE_ARTIFACTS_DIR)/example/src/commands/Build.bs.js

.PHONY: start-example
start-example: ## Start example site in watch mode
	PROJECT_ROOT_DIR=$(MAKEFILE_DIR) RESCRIPT_SSG_BUNDLER=esbuild $(RESCRIPT_SSG_BIN) $(MELANGE_ARTIFACTS_DIR)/example/src/commands/Start.bs.js

.PHONY: serve-example
serve-example: ## Serve example site (use after build)
	$(NODE_BINS)/serve -l 3005 example/build/public

.PHONY: clean-tests
clean-tests: ## Clean test artifacts
	rm -rf tests/output
	rm -rf coverage

.PHONY: tests
tests: clean-tests ## Run tests
	PROJECT_ROOT=$(MAKEFILE_DIR) $(NODE_BINS)/c8 node $(MELANGE_ARTIFACTS_DIR)/tests/Tests.bs.js
