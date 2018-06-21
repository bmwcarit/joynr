/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
module.exports = {
    env: {
        es6: true,
        node: true,
        commonjs: true,
        jasmine: true,
        browser: true
    },
    parserOptions: {
        ecmaVersion: 2017
    },
    extends: ["plugin:promise/recommended", "eslint:recommended"],
    plugins: ["promise"],
    rules: {
        indent: "off",
        quotes: "off",
        "no-empty": "off",
        "linebreak-style": ["error", "unix"],
        semi: ["error", "always"],
        "default-case": "error",
        "no-use-before-define": "error",
        "consistent-return": "off",
        "no-lonely-if": "error",
        "no-console": "error",
        "valid-jsdoc": "off",
        eqeqeq: "error",
        "guard-for-in": "error",
        "no-caller": "error",
        "no-extend-native": "error",
        "no-extra-bind": "error",
        "no-loop-func": "error",
        "no-magic-numbers": "off",
        "no-new": "error",
        "no-new-func": "error",
        "no-proto": "error",
        "no-throw-literal": "error",
        "no-unmodified-loop-condition": "error",
        "no-unused-expressions": "error",
        "no-useless-call": "error",
        "no-useless-concat": "error",
        "prefer-promise-reject-errors": "error",
        "vars-on-top": "error",
        "no-catch-shadow": "error",
        "no-shadow": "off",
        "no-undefined": "off",
        "global-require": "error",
        "no-mixed-requires": "error",
        "no-path-concat": "error",
        "no-sync": "off",
        "no-useless-computed-key": "error",
        "no-useless-constructor": "error",
        "no-var": "error",
        "object-shorthand": "error",
        "prefer-arrow-callback": "error",
        "prefer-const": "error",
        "prefer-destructuring": "off",
        "prefer-template": "error",
        "prefer-spread": "error",
        "promise/always-return": "off",
        "promise/no-return-wrap": "error",
        "promise/param-names": "error",
        "promise/catch-or-return": "error",
        "promise/no-native": "off",
        "promise/no-nesting": "error",
        "promise/no-promise-in-callback": "warn",
        "promise/no-callback-in-promise": "warn",
        "promise/avoid-new": "error",
        "promise/no-new-statics": "off",
        "promise/no-return-in-finally": "off",
        "promise/valid-params": "off"
    },
    globals: { define: false },
    root: true
};
