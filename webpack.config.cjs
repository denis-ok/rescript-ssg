const HtmlWebpackPlugin = require("html-webpack-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");

const path = require("path");

const buildDir = path.join(__dirname, "build");
const outputDir = path.join(buildDir, "bundle");
// const publicDir = path.join(demoDir, "public");

const isProduction = process.env.NODE_ENV === "production";

const pages = require("./build/pages.json");
console.log(pages);
// title
// slug
// entryPath
// outputDir
// htmlTemplatePath

const entries = Object.fromEntries(
  pages.map(({ slug, entryPath }) => [slug, entryPath])
);

const htmlWebpackPlugins = pages.map(
  ({ title, slug, outputDir, htmlTemplatePath }) =>
    new HtmlWebpackPlugin({
      title: title,
      lang: "en",
      // path to html template
      template: htmlTemplatePath,
      // relative path to page?
      filename:
        slug === "index" ? "./index.html" : path.join(`${slug}/index.html`),
      chunks: [slug],
      inject: true,
      // minify html
      minify: false,
    })
);

module.exports = {
  entry: entries,

  mode: isProduction ? "production" : "development",

  output: {
    path: outputDir,
    filename: "js/[name].[chunkhash].js",
    // publicPath: publicPath,
  },

  module: {
    rules: [],
  },

  plugins: [new CleanWebpackPlugin(), ...htmlWebpackPlugins],

  // optimization: {
  //   splitChunks: {
  //     // include all types of chunks
  //     chunks: "all",
  //     // Minimum size, in bytes, for a chunk to be generated.
  //     minSize: 10000,
  //   },
  // },

  devServer: {
    historyApiFallback: true,
    hot: false,
    // static: {
    //   directory: path.join(__dirname, "public"),
    // },
    // compress: true,
    port: 9007,
  },

  // stats: 'verbose'
};
