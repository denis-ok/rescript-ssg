/**
 * ReasonReactHelmet - A pure Reason/Melange implementation of react-helmet
 * Collects meta tags during server-side rendering and provides them via renderStatic()
 */

// Types matching the original react-helmet API
type helmetProperty = {toString: unit => string};

type helmetInstance = {
  base: helmetProperty,
  bodyAttributes: helmetProperty,
  htmlAttributes: helmetProperty,
  link: helmetProperty,
  meta: helmetProperty,
  noscript: helmetProperty,
  script: helmetProperty,
  style: helmetProperty,
  title: helmetProperty,
};

// Internal state types
type tagData = Js.Dict.t(string);

type helmetState = {
  mutable htmlAttributes: array((string, string)),
  mutable bodyAttributes: array((string, string)),
  mutable title: option(string),
  mutable titleAttributes: array((string, string)),
  mutable base: array(tagData),
  mutable meta: array(tagData),
  mutable link: array(tagData),
  mutable script: array(tagData),
  mutable noscript: array(tagData),
  mutable style: array(tagData),
};

let emptyState = (): helmetState => {
  htmlAttributes: [||],
  bodyAttributes: [||],
  title: None,
  titleAttributes: [||],
  base: [||],
  meta: [||],
  link: [||],
  script: [||],
  noscript: [||],
  style: [||],
};

// Module-level mutable state for collecting tags during SSR
let state: ref(helmetState) = ref(emptyState());

// Reset state (useful for testing)
let resetState = () => {
  state := emptyState();
};

// FFI for accessing React element internals
[@mel.get] external getElementType: React.element => Js.Nullable.t(string) = "type";
[@mel.get] external getElementProps: React.element => Js.t({..}) = "props";

// Convert props object to array of key-value pairs
let propsToAttributeArray = (props: Js.t({..})): array((string, string)) => {
  let keys = Js.Obj.keys(props);
  keys->Belt.Array.keepMap(key =>
    if (key == "children") {
      None;
    } else {
      let value = Js.Dict.unsafeGet(Obj.magic(props), key);
      Some((key, Js.String.make(value)));
    }
  );
};

// Convert props object to a dict of strings (for tag data)
// includeChildren: if true, extract children text content as "innerHTML" key
let propsToTagData = (~includeChildren=false, props: Js.t({..})): tagData => {
  let dict = Js.Dict.empty();
  let keys = Js.Obj.keys(props);
  keys->Belt.Array.forEach(key =>
    if (key != "children") {
      let value = Js.Dict.unsafeGet(Obj.magic(props), key);
      Js.Dict.set(dict, key, Js.String.make(value));
    }
  );
  // If includeChildren, extract text content from children
  if (includeChildren) {
    let children = Js.Dict.get(Obj.magic(props), "children");
    switch (children) {
    | Some(c) =>
      let str = Js.String.make(c);
      if (str != "[object Object]") {
        Js.Dict.set(dict, "innerHTML", str);
      };
    | None => ()
    };
  };
  dict;
};

// Extract text content from children (for title tag)
let getChildrenText = (props: Js.t({..})): option(string) => {
  let children = Js.Dict.get(Obj.magic(props), "children");
  switch (children) {
  | Some(c) =>
    // Children could be a string directly or a React element
    let str = Js.String.make(c);
    if (str == "[object Object]") {
      None;
          // It's a React element, try to get its children
    } else {
      Some(str);
    };
  | None => None
  };
};

// Process a single child element
let processChild = (child: React.element): unit => {
  let tagType = getElementType(child)->Js.Nullable.toOption;
  let props = getElementProps(child);

  switch (tagType) {
  | Some("html") =>
    let attrs = propsToAttributeArray(props);
    state^.htmlAttributes = Belt.Array.concat(attrs, state^.htmlAttributes);
  | Some("body") =>
    let attrs = propsToAttributeArray(props);
    state^.bodyAttributes = Belt.Array.concat(attrs, state^.bodyAttributes);
  | Some("title") =>
    let text = getChildrenText(props);
    switch (text) {
    | Some(t) => state^.title = Some(t)
    | None => ()
    };
    let attrs = propsToAttributeArray(props);
    if (Belt.Array.length(attrs) > 0) {
      state^.titleAttributes = attrs;
    };
  | Some("base") =>
    let data = propsToTagData(props);
    state^.base = Belt.Array.concat([|data|], state^.base);
  | Some("meta") =>
    let data = propsToTagData(props);
    state^.meta = Belt.Array.concat([|data|], state^.meta);
  | Some("link") =>
    let data = propsToTagData(props);
    state^.link = Belt.Array.concat([|data|], state^.link);
  | Some("script") =>
    let data = propsToTagData(~includeChildren=true, props);
    state^.script = Belt.Array.concat([|data|], state^.script);
  | Some("noscript") =>
    let data = propsToTagData(~includeChildren=true, props);
    state^.noscript = Belt.Array.concat([|data|], state^.noscript);
  | Some("style") =>
    let data = propsToTagData(~includeChildren=true, props);
    state^.style = Belt.Array.concat([|data|], state^.style);
  | _ => ()
  };
};

// Escape HTML special characters
let escapeHtml = (str: string): string => {
  str
  ->Js.String.replaceByRe(~regexp=[%re "/&/g"], ~replacement="&amp;", _)
  ->Js.String.replaceByRe(~regexp=[%re "/</g"], ~replacement="&lt;", _)
  ->Js.String.replaceByRe(~regexp=[%re "/>/g"], ~replacement="&gt;", _)
  ->Js.String.replaceByRe(~regexp=[%re "/\"/g"], ~replacement="&quot;", _)
  ->Js.String.replaceByRe(~regexp=[%re "/'/g"], ~replacement="&#x27;", _);
};

// Convert attribute array to HTML string (e.g., lang="en" dir="ltr")
let attributesToString = (attrs: array((string, string))): string => {
  attrs->Belt.Array.map(((key, value)) => key ++ "=\"" ++ escapeHtml(value) ++ "\"")->Js.Array.join(~sep=" ", _);
};

// Convert title to HTML string
let titleToString = (title: option(string), attrs: array((string, string))): string => {
  switch (title) {
  | None => ""
  | Some(t) =>
    let attrStr = attributesToString(attrs);
    let attrPart = attrStr == "" ? "" : " " ++ attrStr;
    "<title" ++ attrPart ++ ">" ++ escapeHtml(t) ++ "</title>";
  };
};

// Convert tag data array to HTML string for self-closing tags (meta, link, base)
let selfClosingTagsToString = (tagName: string, tags: array(tagData)): string => {
  tags
  ->Belt.Array.map(tag => {
      let attrs =
        tag
        ->Js.Dict.entries
        ->Belt.Array.map(((key, value)) => key ++ "=\"" ++ escapeHtml(value) ++ "\"")
        ->Js.Array.join(~sep=" ", _);
      "<" ++ tagName ++ " " ++ attrs ++ "/>";
    })
  ->Js.Array.join(~sep="\n", _);
};

// Convert tag data array to HTML string for tags with content (script, noscript, style)
let contentTagsToString = (tagName: string, tags: array(tagData)): string => {
  tags
  ->Belt.Array.map(tag => {
      let entries = tag->Js.Dict.entries;
      // Filter out content-related keys, keep only attributes
      let (attrEntries, contentEntry) =
        entries->Belt.Array.partition(((key, _)) =>
          key != "cssText" && key != "innerHTML" && key != "dangerouslySetInnerHTML"
        );

      let attrs =
        attrEntries
        ->Belt.Array.map(((key, value)) => key ++ "=\"" ++ escapeHtml(value) ++ "\"")
        ->Js.Array.join(~sep=" ", _);

      let content =
        switch (contentEntry->Belt.Array.get(0)) {
        | Some((_, v)) => v
        | None => ""
        };

      let attrPart = attrs == "" ? "" : " " ++ attrs;
      "<" ++ tagName ++ attrPart ++ ">" ++ content ++ "</" ++ tagName ++ ">";
    })
  ->Js.Array.join(~sep="\n", _);
};

// Get the static helmet data and reset state
let renderStatic = (): helmetInstance => {
  let currentState = state^;
  state := emptyState();

  {
    htmlAttributes: {
      toString: () => attributesToString(currentState.htmlAttributes),
    },
    bodyAttributes: {
      toString: () => attributesToString(currentState.bodyAttributes),
    },
    title: {
      toString: () => titleToString(currentState.title, currentState.titleAttributes),
    },
    base: {
      toString: () => selfClosingTagsToString("base", currentState.base),
    },
    meta: {
      toString: () => selfClosingTagsToString("meta", currentState.meta),
    },
    link: {
      toString: () => selfClosingTagsToString("link", currentState.link),
    },
    script: {
      toString: () => contentTagsToString("script", currentState.script),
    },
    noscript: {
      toString: () => contentTagsToString("noscript", currentState.noscript),
    },
    style: {
      toString: () => contentTagsToString("style", currentState.style),
    },
  };
};

// The Helmet component - collects meta tags during render, renders nothing
[@react.component]
let make = (~children: React.element) => {
  // Process children to collect meta tags
  React.Children.forEach(children, child => {processChild(child)});

  // Render nothing - Helmet only collects data
  React.null;
};
