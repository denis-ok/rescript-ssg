const HtmlWebpackPlugin = require("html-webpack-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");

const path = require("path");

const demoDir = path.join(__dirname, "demo");
const outputDir = path.join(demoDir, "bundle");
// const publicDir = path.join(demoDir, "public");

const isProduction = process.env.NODE_ENV === "production";

const pages = [
  {
    title: "Index",
    slug: "index",
    entry: "./demo/Index.bs.js",
    path_: ".",
  },
  {
    title: "Page 1",
    slug: "page1",
    entry: "./demo/Page1.bs.js",
    path_: "page1",
  },
  {
    title: "Page 2",
    slug: "page2",
    entry: "./demo/Page2.bs.js",
    path_: "page2",
  },
];

const entries = Object.fromEntries(
  pages.map(({ slug, entry }) => [slug, entry])
);

const htmlWebpackPlugins = pages.map(
  ({ title, slug, path_ }) =>
    new HtmlWebpackPlugin({
      title: title,
      lang: "en",
      // path to html template
      template: path.join(demoDir, "index-static.html"),
      // output filename
      filename: path.join(`${path_}/index.html`),
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

  optimization: {
    splitChunks: {
      // include all types of chunks
      chunks: "all",
      // Minimum size, in bytes, for a chunk to be generated.
      minSize: 100,
    },
  },

  devServer: {
    historyApiFallback: true,
    // static: {
    //   directory: path.join(__dirname, "public"),
    // },
    // compress: true,
    port: 9007,
  },
};
