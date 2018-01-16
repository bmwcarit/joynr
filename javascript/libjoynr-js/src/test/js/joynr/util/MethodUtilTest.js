/*jslint es5: true, node: true, nomen: true */
/*global fail: true */
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
var MethodUtil = require("../../../classes/joynr/util/MethodUtil");

describe("libjoynr-js.joynr.MethodUtil", function() {
    it("transformParameterMapToArray throws for null values", function() {
        var operationArguments, parameters;
        operationArguments = { bool: null };
        parameters = [{ name: "bool", type: "Boolean", javascriptType: "boolean" }];
        expect(function() {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws for undefined values", function() {
        var operationArguments, parameters;
        operationArguments = { bool: undefined };
        parameters = [{ name: "bool", type: "Boolean", javascriptType: "boolean" }];
        expect(function() {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match number of arguments", function() {
        var operationArguments, parameters;
        operationArguments = {};
        parameters = [{ name: "bool", type: "Boolean", javascriptType: "boolean" }];
        expect(function() {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match arguments (name mismatch)", function() {
        var operationArguments, parameters;
        operationArguments = { int: true };
        parameters = [{ name: "bool", type: "Boolean", javascriptType: "boolean" }];
        expect(function() {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match arguments (type mismatch)", function() {
        var operationArguments, parameters;
        operationArguments = { bool: 2 };
        parameters = [{ name: "bool", type: "Boolean", javascriptType: "boolean" }];
        expect(function() {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray can convert map to array", function() {
        var expectedObject, operationArguments, parameters;
        expectedObject = { paramDatatypes: ["Boolean"], params: [true] };
        operationArguments = { bool: true };
        parameters = [{ name: "bool", type: "Boolean", javascriptType: "boolean" }];
        var result = MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        expect(result).toEqual(expectedObject);
    });
});
