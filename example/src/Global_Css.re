let injectGlobal = () => [%styled.global
  {|
    body {
      font-size: 16px;
      background-color: #FADCD9;
      padding: 32px;
    }

    h1, h2, h3, h4, h5, h6 {
      margin: 0;
      font-weight: normal;
    }

    a {
      color: black;
    }

    a:hover {
      color: violet;
    }
  |}
];
