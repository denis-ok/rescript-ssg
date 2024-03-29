// Unused bindings commented out

// type cache;

// module CacheProvider = {
//   [@react.component] [@bs.module "@emotion/react"]
//   external make: (~value: cache, ~children: React.element) => React.element =
//     "CacheProvider";
// };

// module Css = {
//   [@bs.module "@emotion/css"] external defaultCache: cache = "cache";
// };

// Custom cache doesn't work for some reason. But is it needed? Seems not.
// Looks like the same issue: https://github.com/emotion-js/emotion/issues/2731
// Default cache works fine.

// module Cache = {
//   type createCacheInput = {key: string};
//   [@bs.module "@emotion/cache/dist/emotion-cache.cjs.js"] [@bs.scope "default"]
//   external createCache: createCacheInput => cache = "default";
// };

module Server = {
  type extractCriticalResult = {
    html: string,
    css: string,
    ids: array(string),
  };

  // type createEmotionServerResult = {
  //   extractCritical: string => extractCriticalResult,
  // };

  // All exports from "@emotion/server" index are the results of internal calling "createEmotionServer(cache)"
  // where passed cache is default cache imported from "@emotion/css".

  [@bs.module "@emotion/server"]
  external extractCritical: string => extractCriticalResult =
    "extractCritical";

  [@bs.module "@emotion/server"]
  external renderStylesToString: string => string = "renderStylesToString";
  // Below is a function to build emotion server manually with a custom cache.
  // [@bs.module
  //   "@emotion/server/create-instance/dist/emotion-server-create-instance.cjs.js"
  // ]
  // [@bs.scope "default"]
  // external createEmotionServer: cache => createEmotionServerResult = "default";
};
