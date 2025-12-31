# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds the library sources; `src/bindings/` contains JS/Node bindings and externals.
- `tests/` holds test modules and fixtures; the entry module is `tests/Tests.re`.
- `example/` is a full demo site (`example/src/`), including CSS and static assets under `example/src/images/`.
- `_build/`, `coverage/`, and `example/build/` are generated outputs and should not be edited directly.

## Build, Test, and Development Commands
- `make init` sets up the local toolchain (opam switch, npm deps, opam deps).
- `make build` builds the library via `dune`.
- `make watch` runs `dune` in watch mode for fast iteration.
- `make tests` runs the JS/Melange test runner with coverage via `c8`.
- `make build-example` builds the demo site; `make start-example` runs it in watch mode.
- `make serve-example` serves the built demo site on port 3005.

## Coding Style & Naming Conventions
- Use existing Reason formatting. Run `make format` to apply `dune`/`ocamlformat` rules.
- Keep module names in `PascalCase` (e.g., `PageBuilder.re`), and prefer descriptive filenames that match the module name.
- Follow current directory conventions: core logic in `src/`, bindings in `src/bindings/`.

## Testing Guidelines
- Tests are written in Reason/Melange under `tests/`, with fixtures in `tests/fixtures/`.
- Prefer adding new test modules next to `tests/Tests.re` and wire them into the main test entry.
- Use `make test` for dune-level checks and `make tests` for JS runtime/coverage; outputs land in `coverage/` and `tests/output/`.

## Commit & Pull Request Guidelines
- Commit messages in history are short, lowercase, and imperative (e.g., "update chokidar"). Keep them focused.
- PRs should include a clear description, relevant commands run (e.g., `make test`), and screenshots when changing the example site output.

## Configuration Notes
- Tooling is split across `opam`/`dune` (OCaml/ReScript) and `npm` (JS tools). Keep both lockfiles and build outputs consistent.
