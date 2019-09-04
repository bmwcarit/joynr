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
        jest: true,
        browser: true
    },
    parser: "@typescript-eslint/parser",
    parserOptions: {
        ecmaVersion: 2018,
        sourceType: "module"
    },
    extends: [ // order matters. Later entries override earlier entries when conflicts arise
        "eslint:recommended",
        "plugin:promise/recommended",
        "plugin:prettier/recommended",
        "plugin:@typescript-eslint/recommended",
        "plugin:jest/recommended",
        "prettier/@typescript-eslint"
    ],
    plugins: ["promise", "jest"],
    rules: {
        indent: "off",
        quotes: "off",
        "no-empty": "off",
        "no-extra-semi": "off",
        "no-mixed-spaces-and-tabs": "off",
        "no-unexpected-multiline": "off",
        "default-case": "error",
        "no-use-before-define": "off",
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
        "vars-on-top": "off",
        "no-catch-shadow": "error",
        "no-shadow": "off",
        "no-undefined": "off",
        "global-require": "error",
        "no-mixed-requires": "error",
        "no-path-concat": "error",
        "no-sync": "off",
        "no-useless-computed-key": "error",
        "no-useless-constructor": "off",
        "no-var": "error",
        "no-redeclare": "off",
        "no-dupe-class-members": "off",
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
        "promise/valid-params": "off",
        "@typescript-eslint/no-use-before-define": ["error", { "functions": false }],
        "@typescript-eslint/no-namespace": "off",
        "@typescript-eslint/no-parameter-properties": "off",
        "@typescript-eslint/no-explicit-any": "off",
        "@typescript-eslint/no-unused-vars": "off",
        "@typescript-eslint/explicit-function-return-type": ["error", {
            allowExpressions: true
        }]
    },
    overrides: [
        {
            files: ["*Test.ts", "*.spec.ts"],
            rules: { "@typescript-eslint/explicit-function-return-type": "off",
                "no-new": "off" }
        }
    ],
    globals: { define: false },
    root: true
};
