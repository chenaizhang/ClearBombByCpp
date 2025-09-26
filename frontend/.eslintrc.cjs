module.exports = {
  root: true,
  env: {
    browser: true,
    es2021: true
  },
  extends: ["eslint:recommended", "plugin:react/recommended"],
  plugins: ["react", "react-hooks"],
  parserOptions: {
    ecmaVersion: "latest",
    sourceType: "module"
  },
  rules: {
    "react/react-in-jsx-scope": "off",
    "react/prop-types": "off"
  },
  settings: {
    react: {
      version: "detect"
    }
  }
};
