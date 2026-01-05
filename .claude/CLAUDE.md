# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds the library sources. Mostly OCaml/Reason code and a bit of JS; `src/bindings/` contains JS/Node bindings.
- `tests/` holds test modules and fixtures;
- `example/` is a demo project build with this library.
- `_build/`, `coverage/`, and `example/build/` are generated outputs and should not be edited directly.

## Build, Test, and Development Commands
- `make build` compailes the library (OCaml/Reason) sources using OCaml/Melange compiler with `dune`.
- `make watch` runs `dune` in watch mode.
- `make tests` runs the JS/Melange test runner.
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
