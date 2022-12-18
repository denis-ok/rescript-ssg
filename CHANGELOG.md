## 1.9.0
- Put bundle `.js` files to `assets/js` dir.
- Fix issues with `asset-prefix` parameter.
- Initialize tests. https://github.com/denis-ok/rescript-ssg/commit/89a831147f487923db83e78c55bb797008daca60
- Update dependencies, remove unused `webpack-cli` dep.
- Provide env vars to browser via webpack define plugin. https://github.com/denis-ok/rescript-ssg/commit/3222bad9c39a895a928c5aaa30c493ef69944ff3
- Add `writeWebpackStatsJson` flag to disable writing webpack `stats.json`. Now disabled by default.
- Catch error in `RebuildPageWorker` to avoid stopping of Webpack/watcher after renaming files. https://github.com/denis-ok/rescript-ssg/commit/cde6b81b09db11f552b9d0f1ef100fbc82c8c659
- Allow choosing minimizer for production bundle build. `Terser` is Webpack default, `Esbuild` is an alternative/experimental option added via `esbuild-loader` (it's fast). https://github.com/denis-ok/rescript-ssg/commit/21eb9d450d603267bcbd5742bf0e412591fae991

## 1.8.0

- Inject emotion styles to `<head>` instead on inline [Commit](https://github.com/denis-ok/rescript-ssg/commit/aa2a47b254a2ca0ffc33f5cad0a4d7ae4b2a1176).
- Build React App files with ReScript syntax.
- Remove `webpackOutputDir` param [Commit](https://github.com/denis-ok/rescript-ssg/commit/e051e769cfdaed50ec2ef4dcf9a4a5b5b23e4e20).
- Tweak webpack chunks [Commit](https://github.com/denis-ok/rescript-ssg/commit/2a9d44a8f398a6f721eeabbd7e1b048efd33d252).

## 1.7.0

- Add `headCssFilepaths` field to inject CSS to page's `<head>`, [PR](https://github.com/denis-ok/rescript-ssg/pull/8).
- Refactor watcher, rebuild pages on demand with a debounced function instead of checking queue with `setInterval`, [Commit](https://github.com/denis-ok/rescript-ssg/commit/b5331109834d998cef144c1d26bc7f995accebd6).
- Improve/refactor logging, add `logLevel` parameter to build/start commands, [PR](https://github.com/denis-ok/rescript-ssg/pull/9).

## 1.6.0

- Add `headCss` field to page to inject CSS into result HTML.
- Minify HTML in production mode.
- Refactor node-loader.
- Reorganize modules, remove unused code, some refactoring.

## 1.5.0

- Add `aggregateTimeout` option to Webpack config to avoid too often rebuilding.
- Move some dependencies to peerDependencies.
- Bump `webpack-dev-server` dependency.

## 1.4.0

- Format files, add `.css` to asset regex.

## 1.3.0

- Fix path to node loader in binary.

## 1.2.0

- Make node-loader compatible with prev node versions.

## 1.1.0

- Add binary file that runs node with experimental loader.

## 1.0.0

- Switch to major version.
