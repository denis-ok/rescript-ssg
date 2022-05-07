// Unused bindings commented out

// type cache;

// module CacheProvider = {
//   [@react.component] [@bs.module "@emotion/react"]
//   external make: (~value: cache, ~children: React.element) => React.element =
//     "CacheProvider";
// };

// module Css = {
//   [@module "@emotion/css"] external defaultCache: cache = "cache";
// };

// Custom cache doesn't work for some reason. But is it needed? Not sure.
// Default cache work fine.
// Looks like the same issue: https://github.com/emotion-js/emotion/issues/2731

// module Cache = {
//   type createCacheInput = {key: string};

//   [@module "@emotion/cache/dist/emotion-cache.cjs.js"] [@scope "default"]
//   external createCache: createCacheInput => cache = "default";
// };

module Server = {
  // type extractCriticalResult = {
  //   html: string,
  //   css: string,
  //   ids: array(string),
  // };

  // type createEmotionServerResult = {
  //   //
  //   extractCritical: string => extractCriticalResult,
  // };

  // All exports from "@emotion/server" index are the results of internal calling "createEmotionServer(cache)"
  // where passed cache is default cache imported from "@emotion/css".

  // [@module "@emotion/server"]
  // external extractCritical: string => extractCriticalResult =
  //   "extractCritical";

  [@module "@emotion/server"]
  external renderStylesToString: string => string = "renderStylesToString";

  // Below is a function to build emotion server manually with a custom cache.
  // [@module
  //   "@emotion/server/create-instance/dist/emotion-server-create-instance.cjs.js"
  // ]
  // [@scope "default"]
  // external createEmotionServer: cache => createEmotionServerResult = "default";
};
