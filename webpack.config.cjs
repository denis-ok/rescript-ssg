const HtmlWebpackPlugin = require("html-webpack-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");

const path = require("path");

const demoDir = path.join(__dirname, "demo");
const outputDir = path.join(demoDir, "bundle");
const publicDir = path.join(demoDir, "public");

const production = process.env.NODE_ENV === "production";

module.exports = {
  entry: path.join(demoDir, "Index.bs.js"),

  mode: production ? "production" : "development",

  output: {
    path: outputDir,
    filename: "bundle.js",
  },

  module: {
    rules: [],
  },

  plugins: [
    new CleanWebpackPlugin(),
    new HtmlWebpackPlugin({
      template: path.join(demoDir, "index-static.html"),
    }),
  ],

  devServer: {
    historyApiFallback: true,
    // static: {
    //   directory: path.join(__dirname, "public"),
    // },
    // compress: true,
    port: 9007,
  },
};
